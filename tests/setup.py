from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

example_module = Pybind11Extension(
    'madpod',
    [str(fname) for fname in Path('../').glob('*.c')],
    include_dirs=['../'],
    extra_compile_args=['-O3']
)

setup(
    name='madpod',
    version=0.1,
    author='VP',
    author_email='vp@vp.com',
    description='madpod bindings',
    ext_modules=[example_module],
    cmdclass={"build_ext": build_ext},
)