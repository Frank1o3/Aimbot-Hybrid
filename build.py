import os
import sys
import subprocess
import hashlib
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = ""):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: CMakeExtension):
        # Target: root/src/imgbuffer/
        # We use ext.name here. If name is "imgbuffer", it goes to src/imgbuffer
        ext_fullpath = Path(self.get_ext_fullpath(ext.name)).resolve()
        extdir = ext_fullpath.parent
        extdir.mkdir(parents=True, exist_ok=True)

        if not self.needs_rebuild(ext):
            print(f"✓ {ext.name} is up to date")
            return

        print(f"Building {ext.name}...")
        cfg = "Debug" if self.debug else "Release"
        build_temp = Path(self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",
        ]

        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(
            ["cmake", "--build", "."] + ["--config", cfg], cwd=build_temp
        )

        # Create an __init__.py in the extension dir so it's a valid package
        init_py = extdir / "__init__.py"
        init_py.touch(exist_ok=True)

        self.save_build_hash(ext)
        self.generate_stubs(extdir, ext.name)

    def generate_stubs(self, output_dir: Path, ext_name: str):
        """
        Generate stubs for all compiled extension modules in output_dir.
        """
        print(f"Generating stubs in {output_dir}...")

        env = os.environ.copy()

        # Ensure package root is importable
        src_root = output_dir.parent
        env["PYTHONPATH"] = str(src_root) + os.pathsep + env.get("PYTHONPATH", "")

        package = output_dir.name  # e.g. "capture"

        # Extension suffixes to look for
        suffixes = {".so", ".pyd", ".dll"}

        for file in output_dir.iterdir():
            if not file.is_file():
                continue

            if file.suffix not in suffixes:
                continue

            # Remove ABI suffix: capture.cpython-313-x86_64-linux-gnu.so → capture
            module_base = file.name.split(".", 1)[0]

            full_module_name = f"{package}.{module_base}"

            try:
                print(f"  → stubgen {full_module_name}")
                subprocess.check_call(
                    [
                        sys.executable,
                        "-m",
                        "pybind11_stubgen",
                        full_module_name,
                        "-o",
                        "src",
                    ],
                    env=env,
                )
            except subprocess.CalledProcessError as e:
                print(f"⚠ Failed to generate stubs for {full_module_name}: {e}")


    def needs_rebuild(self, ext: CMakeExtension) -> bool:
        ext_path = Path(self.get_ext_fullpath(ext.name))
        if not ext_path.exists():
            return True
        current_hash = self.calculate_cpp_hash(Path(ext.sourcedir))
        hash_file = Path(self.build_temp) / ".cpp_hash"
        return not hash_file.exists() or hash_file.read_text().strip() != current_hash

    def calculate_cpp_hash(self, cpp_dir: Path) -> str:
        hasher = hashlib.md5()
        for pattern in ("**/*.cpp", "**/*.hpp", "**/*.h", "**/CMakeLists.txt"):
            for path in sorted(cpp_dir.glob(pattern)):
                if path.is_file() and "_deps" not in path.parts:
                    hasher.update(path.read_bytes())
        return hasher.hexdigest()

    def save_build_hash(self, ext: CMakeExtension):
        hash_file = Path(self.build_temp) / ".cpp_hash"
        hash_file.parent.mkdir(parents=True, exist_ok=True)
        hash_file.write_text(self.calculate_cpp_hash(Path(ext.sourcedir)))


if __name__ == "__main__":
    setup(
        name="aimbot-workspace",
        version="0.1.1",
        packages=["aimbot", "imgbuffer", "capture"],
        package_dir={"": "src"},
        ext_modules=[CMakeExtension("capture.capture", sourcedir="cpp")],
        cmdclass={"build_ext": CMakeBuild},
        zip_safe=False,
    )
