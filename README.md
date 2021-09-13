# lasHemisphericSIM
Provides a simulated hemispherical photograph at user-defined positions in a point cloud. 

## Installation instruction

### Install

#### LINUX 
 - Make sure you have laslib (from LAStools) and libgeotiff in the system. 
 - Download "lasHemisphericSIM" 
 - go to terminal and run 
    
        lasHemispheriSIM -h


    
    
### Compile from source   

If you want to compile from source, add these files in the "example" directory of LASlib in LAStools. 

Open "CMakeLists.txt" and add "lasHemisphericSIM" in the "set(TARGETS...)" chunk. E.g.:
         
      set(TARGETS
      	lasexample
      	**lasHemisphericSIM**
      	lasexample_write_only
      	lasexample_add_rgb
      	lasexample_simple_classification
      	lasexample_write_only_with_extra_bytes)

Recompile LASlib following instructions in LAStools (e.g. cmake ../ and make) .

## Usage and description

This tools basically estimates how much direct light arrives at a certain spot/plot. User provides a CSV file with a list of coordinates and the tool converts the point cloud coordinates to polar coordinates and figures how much obstruction they create, by testing a number of "line of sight" directions that would correspond to pixels of a photo at zenithal (upward to the sky) direction using  fish-eye lens.   

 
### Canopy and vegetation
   
Please make sure you use a **normalized** point cloud, in LAS or LAZ format, if you want to focus on canopy light transmission.


## Acknowledgements

This tool was created in the context of two projects, 

CONAF ...
VARCITIES ... 

## Contacts   

Contact <a href=mailto:francesco.pirotti@unipd.it>AUTHOR</a> or visit https://www.cirgeo.unipd.it for more info and links to social-media. 