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

If you want to compile from source, compile first LAStools/LASlib add these files in the "example" directory of LASlib in LAStools. 

Go to the "example" directory (<LAStools install dir>/LASlib/example).  
Open "Makefile" and modify: 
         
      all: lasexample lasexample_write_only lasexample_add_rgb lasexample_simple_classification lasexample_write_only_full_waveform lasexample_write_only_with_extra_bytes

to 

      all: lasexample lasHemisphericSIM lasexample_write_only lasexample_add_rgb lasexample_simple_classification lasexample_write_only_full_waveform lasexample_write_only_with_extra_bytes

and right after add:

      lasHemisphericSIM: lasHemisphericSIM.o
      	${LINKER} ${BITS} ${COPTS} lasHemisphericSIM.o -llas   -o $@ ${LIBS} ${LASLIBS} $(INCLUDE) $(LASINCLUDE)


You should be able then to run successfully "make lasHemisphericSIM" in the directory and create the executable.



## Usage and description

This tools basically estimates how much direct light arrives at a certain spot/plot. User provides a CSV file with a list of coordinates and the tool converts the point cloud coordinates to polar coordinates and figures how much obstruction they create, by testing a number of "line of sight" directions that would correspond to pixels of a photo at zenithal (upward to the sky) direction using  fish-eye lens.   

*PARAMETERS*

**-loc \<file path\>**: is the path to a CSV file with X Y coordinates - with header - other columns can be present and will be saved in output. Comma, tab, pipe, space, column and semi-column characters are accepted as column separators.

  **Example of CSV file contents**  
  
    X;Y
    726836.4;5140271.4
    726994.6;5140427.6
    726861.51;5140416.72
    726850.40;5140148.38
    726717.3;5140225.1
    726715.63;5140525.64



**-zCam \<height value in meters\>**: *default=1.3m* \n\t- height of camera - NB this is in absolute height with respect to the point cloud, so if your point cloud is normalized (e.g. a canopy height model) then 1.3m will be 1.3m from the ground.

    -zCam 1.3


**-zenCut \<Zenith angle in degrees\>**: *default=89°* \n\t- At 90° the points will be at the horizon, potentially counting million of \n\tpoints: a smaller Zenith angle will ignore points lower than that angle \n\t(e.g. at 1km distance any point lower than 17.5 m height with respect to the camera ) . 

    -zenCut 89

**Examples** 


    lasHemisphericSIM -i /archivio/LAS/las_normalized/BL5_UTM_32_ort_1138.laz -loc BL5_Ortofoto_UTM_32_1138.csv -verbose


### Canopy and vegetation
   
Using a **normalized** point cloud is more straight forward, if you want to focus on canopy light transmission.


## Acknowledgements

This tool was created in the context of two projects, 

CONAF ...
VARCITIES ... 

## Contacts   

Contact <a href=mailto:francesco.pirotti@unipd.it>AUTHOR</a> or visit https://www.cirgeo.unipd.it for more info and links to social-media. 