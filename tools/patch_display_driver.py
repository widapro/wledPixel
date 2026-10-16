"""Add a transmission-only rotation hook to the pinned MD_MAX72XX driver.

Generate a build-local source copy, leaving dependency files untouched. This
keeps Parola, WOPR, progress bars and boot messages in logical coordinates.
"""
from pathlib import Path


def patched_driver(source, rotation_header):
    start = "void MD_MAX72XX::spiSend(void)\n{\n"
    end = "    _spiRef.endTransaction();\n#endif\n}"
    if source.count(start) != 1 or source.count(end) != 1:
        raise RuntimeError("MD_MAX72XX spiSend changed; review the rotation hook")
    source = '#include "' + Path(rotation_header).as_posix() + '"\n' + source
    source = source.replace(start, start + """  const bool rotateOutput = displayRotation::flipped;
  const uint8_t rotateModules = displayRotation::moduleCount;
  if (rotateOutput)
    displayRotation::transformSpiBuffer(_spiData, _maxDevices, rotateModules);
""")
    return source.replace(end, """    _spiRef.endTransaction();
#endif
  if (rotateOutput)
    displayRotation::transformSpiBuffer(_spiData, _maxDevices, rotateModules);
}""")


def install(env):
    def patch_source(build_env, node):
        original = Path(node.srcnode().get_abspath())
        output = Path(build_env.subst("$BUILD_DIR")) / "display_rotation" / original.name
        content = patched_driver(
            original.read_text(),
            Path(build_env.subst("$PROJECT_DIR")) / "src" / "DisplayRotation.h",
        )
        output.parent.mkdir(parents=True, exist_ok=True)
        if not output.exists() or output.read_text() != content:
            output.write_text(content)
        build_env.AppendUnique(CPPPATH=[str(original.parent)])
        return build_env.File(str(output))

    env.AddBuildMiddleware(patch_source, "*/MD_MAX72XX/src/MD_MAX72xx.cpp")


try:
    Import("env")  # noqa: F821 - PlatformIO/SCons
except NameError:
    pass  # Also importable by native tests.
else:
    install(env)  # noqa: F821
