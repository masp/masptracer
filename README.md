# Compiling and Running
```
mkdir build
cd build
cmake ..
make
masptracer ../input.dimensions
```

# Usage
Generating a sample PPM file `outputfile` using input dimension file `inputfile` with generator `gradient`:
```
masptracer <inputfile> [-o outputfile] [-g gradient/mandel]
```

The `-o` and `-g` options are optional. The input file is mandatory.