# lasHemisphericSIM
Provides a simulated hemispherical photograph at user-defined positions in a point cloud. 

## Compilation

If you want to compile from source, add these files in the "example" directory of LASlib in LAStools. 

Open "CMakeLists.txt" and add "lasHemisphericSIM" in the "set(TARGETS...)" chunk. E.g.:
         
      set(TARGETS
      	lasexample
      	**lasHemisphericSIM**
      	lasexample_write_only
      	lasexample_add_rgb
      	lasexample_simple_classification
      	lasexample_write_only_with_extra_bytes)

Recompile LASlib following instructions in LAStools.

## Compilation
