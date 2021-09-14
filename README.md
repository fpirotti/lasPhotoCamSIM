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

*to *

  all: lasexample **lasHemisphericSIM** lasexample_write_only lasexample_add_rgb lasexample_simple_classification lasexample_write_only_full_waveform lasexample_write_only_with_extra_bytes

*and right after add:*

lasHemisphericSIM: lasHemisphericSIM.o
      	${LINKER} ${BITS} ${COPTS} lasHemisphericSIM.o -llas   -o $@ ${LIBS} ${LASLIBS} $(INCLUDE) $(LASINCLUDE)


You should be able then to run successfully **"make lasHemisphericSIM"** in the directory and this creates the executable.

For **windows** the MingW co



## Usage and description

This tools basically estimates how much direct light arrives at a certain spot/plot. User provides a CSV file with a list of coordinates and the tool converts the point cloud coordinates to polar coordinates and figures how much obstruction they create, by testing a number of "line of sight" directions that would correspond to pixels of a photo at zenithal (upward to the sky) direction using  fish-eye lens.   

### **PARAMETERS**

**-loc \<file path\>**: is the path to a CSV file with X Y coordinates - with header - other columns can be present and will be saved in output. Comma, tab, pipe, space, column and semi-column characters are accepted as column separators.

Example of CSV file contents from a file names *cameras.csv*:   
  
    X;Y
    726836.4;5140271.4
    726994.6;5140427.6
    726861.51;5140416.72
    726850.40;5140148.38
    726717.3;5140225.1
    726715.63;5140525.64


Example of **ouput** CSV file contents; the output file will be names *cameras.csv.out* :
  
    X;Y;GapFraction
    726836.4;5140271.4;23.5
    726994.6;5140427.6;66.9
    726861.51;5140416.72;11.6
    726850.40;5140148.38;99.4
    726717.3;5140225.1;56.7
    726715.63;5140525.64;33.6



**-orast**: exports 180x180 pixel rasters in ESRI GRID ASCII format. Pixels represent the point counts.   

**-log10**: converts pixel values, which represent point counts, to log10 scale (-orast must be also present) - formula is log10(pixelvalue+1). 
Cells with no pixels (value=0) are thus given log(1) and have value 0 also after transformation.  This can be helpful as high zenith angles will obviously intersect a very high number of poitnts. Log-transformation can scale to better visualize results. 

**-db**: converts to dB (decibel values) with -10*log10(pixelvalue/maxPixelValue). The pixel with most point counts will have value 1, the other will have positive values.  

**-weight \<power value\>**: *default=0.0* power of inverse distance weight. Each point that intersects a 1°x1° sector in the dome will be positioned at a certain distance that can be used to weight the value of the point. No weight=each point weights 1, i.e. if 10 points are in the sector, that sector will be occluded by 10 points. If weight=2 (-weight 2.0) is provided, each point will add a value of 1.0 * 1.0/pow(distance, 2.0) to the total.  Default value is 0 because no weight is applied. 

**-zCam \<height value in meters\>**: *default=1.3m* \n\t- height of camera - NB this is in absolute height with respect to the point cloud, so if your point cloud is normalized (e.g. a canopy height model) then 1.3m will be 1.3m from the ground.

    -zCam 1.3


**-zenCut \<Zenith angle in degrees\>**: *default=89°* \n\t- At 90° the points will be at the horizon, potentially counting million of \n\tpoints: a smaller Zenith angle will ignore points lower than that angle \n\t(e.g. at 1km distance any point lower than 17.5 m height with respect to the camera ) . 

    -zenCut 89



**Examples** 


    lasHemisphericSIM -i /archivio/LAS/las_normalized/forest.laz -loc cameras.csv -verbose
    
Will read all points from forest.laz and camera locations at cameras.csv file in current directory, providing verbose messages.


    lasHemisphericSIM -i /archivio/LAS/las_normalized/*.laz -loc camera.csv -log -weight 2.0 -verbose


Will read points in all LAZ files in folder /archivio/LAS/las_normalized/   and camera locations at cameras.csv file with verbose messages.


### Canopy and vegetation
   
Using a **normalized** point cloud is more straight forward, if you want to focus on canopy light transmission.




## Acknowledgements

This tool was created in the context of two projects, 

CONAF ...
VARCITIES ... 

## Contacts   

Contact <a href=mailto:francesco.pirotti@unipd.it>AUTHOR</a> or visit https://www.cirgeo.unipd.it for more info and links to social-media. 