from setuptools import setup, Extension

module = Extension(
    'pcinfo',
    sources=['info.c'],
    libraries=['Advapi32', 'Ws2_32'],
)

setup(
    name='pcinfo',
    version='0.1',
    description='C module that provides PC information',
    ext_modules=[module],
)
