!IFDEF OS
PYTHON = python
RM = del /Q
RMDIR = rmdir /S /Q
!ELSE
PYTHON = python3
RM = rm -f
RMDIR = rm -rf
!ENDIF

install:
	pip install .

build:
	$(PYTHON) setup.py build_ext --inplace

run:
	$(PYTHON) main.py

clean:
	-$(RMDIR) build dist __pycache__ *.egg-info
	-$(RM) *.pyo *.pyd *.obj *.lib *.exp *.dll *.o *.so *exe *.pdb *.ilk 
