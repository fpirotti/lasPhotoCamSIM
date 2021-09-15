# lasHemisphericSIM


Provides a simulated hemispherical photograph at user-defined positions in a point cloud. 

<img src="./img1.jpg" />

## Installation instruction

### Install

Download the compiled executable lasHemisphericSIM (linux) or lasHemisphericSIM.exe (Windows 64-bit compiled with MingW) and enjoy.


### Compile

To compile from source code clone this directory or download the CPP and HPP files and follow these instructions.

#### LINUX   

 - Download LAStools (https://rapidlasso.com/lastools/)[https://rapidlasso.com/lastools/]. 
 - Compile LAStools/LASlib :
    - go to LAStools directory and create a "build" directory (e.g. mkdir build)
    - cd build
    - cmake ../ (install cmake if you don't have it) 
    - make
 - Compile lasHemisphericSIM :
    - go to the "example" directory in LASlib (<LAStools install dir>/LASlib/example) and copy/move the lasHemisphericSIM.cpp and lasHemisphericSIM.hpp files there.
    - Open "Makefile" file and modify contents: 


*convert*          

  _all: lasexample lasexample_write_only lasexample_add_rgb lasexample_simple_classification lasexample_write_only_full_waveform lasexample_write_only_with_extra_bytes_

*to *

  _all: lasexample lasHemisphericSIM lasexample_write_only lasexample_add_rgb lasexample_simple_classification lasexample_write_only_full_waveform lasexample_write_only_with_extra_bytes_

*and right after add:*

_lasHemisphericSIM: lasHemisphericSIM.o_   
      	_${LINKER} ${BITS} ${COPTS} lasHemisphericSIM.o -llas   -o $@ ${LIBS} ${LASLIBS} $(INCLUDE) $(LASINCLUDE)_


You should be able then to run successfully the command **"make lasHemisphericSIM"** in the directory and this creates the executable.


### WINDOWS

Use  the MingW compiler chain and follow the steps above, like for linux platforms.



## Usage and description

This tools basically estimates how much light arrives at a certain spot/plot., or how much open skype (aka gap-fraction in forests), or complementary canopy cover ratio, is present at a certain spot. User provides a CSV file with a list of coordinates that correspond to camera positions, height of camera position,  and the tool reprojects the  points from the point cloud to spherical coordinates with respect to a hemispherical dome around the camera,  and figures how much obstruction they create, by testing 1° azimuth x 0.5° zenith sectors, rays that are traced between camera center and points.   

### PARAMETERS

**-loc \<file path\>**: is the path to a CSV file with X Y coordinates of camera locations - with header - other columns can be present and will be saved in output. Comma, tab, pipe, space, column and semi-column characters are accepted as column separators.

Example: **-loc cameras.csv** cameras.csv with the following contents:   
  
    X;Y
    726836.4;5140271.4
    726994.6;5140427.6
    726861.51;5140416.72
    726850.40;5140148.38
    726717.3;5140225.1
    726715.63;5140525.64


After running, the ouput will create a file in the same directory **cameras.csv.out** with the following contents:
  
    X;Y;GapFraction
    726836.4;5140271.4;23.5
    726994.6;5140427.6;66.9
    726861.51;5140416.72;11.6
    726850.40;5140148.38;99.4
    726717.3;5140225.1;56.7
    726715.63;5140525.64;33.6


**-orast**: exports reprojected shperical coordinates to a planar 180x180 (default) ESRI GRID ASCII format. Pixels represent the point counts. For larger grids see the following argument **-mult**.  The cell values are the counts of points  

**-mult \<multiplier value\>**: *default=1* the output raster is 180x180 pixels, if you add "-mult 2" it will become 360x360 .... NB this does not influence the gap fraction calculation, which is fixed on a hemispheric dome divided in sectors 1° in azimunt and 0.5° in zenith angles. 

**-log10**: converts pixel values, which represent point counts, to log10 scale (-orast must be also present) - formula is log10(pixelvalue+1). 
Cells with no pixels (value=0) are thus given log(1) and have value 0 also after transformation.  This can be helpful as high zenith angles will obviously intersect a very high number of poitnts. Log-transformation can scale to better visualize results. 

**-db**: converts to dB (decibel values) with -10*log10(pixelvalue/maxPixelValue). The pixel with most point counts will have value 1, the other will have positive values.  

**-weight \<power value\>**: *default=0.0* power of inverse distance weight. Each point that intersects a 1°x1° sector in the dome will be positioned at a certain distance that can be used to weight the value of the point. No weight=each point weights 1, i.e. if 10 points are in the sector, that sector will be occluded by 10 points. If weight=2 (-weight 2.0) is provided, each point will add a value of 1.0 * 1.0/pow(distance, 2.0) to the total.  Default value is 0 because no weight is applied. 

**-zCam \<height value in meters\>**: *default=1.3m* \n\t- height of camera - NB this is in absolute height with respect to the point cloud, so if your point cloud is normalized (e.g. a canopy height model) then 1.3m will be 1.3m from the ground. 

**-zenCut \<Zenith angle in degrees\>**: *default=89* \n\t- At zero (0) degrees zenith angle the direction points directly upwards, at 90 degrees it points at the horizong.  Thus 90 degrees zenith angle potentially will intercept  million of points: a smaller Zenith angle will ignore points lower than that angle (e.g. setting at 85 degrees will cut-off points that are below 5° from the horizon) . 

### OUTPUT

  1. a CSV file with the file name  appended with  *.out* and file contents of original camera locations file with appended column with gap fraction values.
  2. If *-orast*  is set, one ESRI GRID ASCII (.asc) text file that can be loaded in a gis software. One grid file per plot, named <plot. Center of grid is geolocated at camera position, but of course the size is not scaled.
 
**Examples** 


    lasHemisphericSIM -i /archivio/LAS/las_normalized/forest.laz -loc cameras.csv -verbose
    
Will read all points from forest.laz and camera locations at cameras.csv file in current directory, providing verbose messages.


    lasHemisphericSIM -i /archivio/LAS/las_normalized/*.laz -loc camera.csv -log -weight 2.0 -verbose

Will read points in all LAZ files in folder /archivio/LAS/las_normalized/   and camera locations at cameras.csv file with verbose messages, and count points in falling in spherical sectors applying an inverse distance weight that is a power of 2. E.g. a point that is at distance X will count 1/X^2. **A 5 cm minimum distance is applyed** to avoid overflow of value if by chance a point is at 0.0m distance - this is reasonable as when camera is positioned, the operator will make sure that there not an obstruction right on the same position as the camera (e.g. right under a leaf).
 

### Canopy and vegetation
   
Using a **normalized** point cloud is more straight forward, if you want to focus on canopy light transmission.


## Acknowledgements

This tool was created in the context of two projects, 

CONAF ...
VARCITIES ... 

## Contacts   

Contact <a href=mailto:francesco.pirotti@unipd.it>AUTHOR</a> or visit  [https://www.cirgeo.unipd.it](https://www.cirgeo.unipd.it)
for more info and links to social-media, [Facebook](https://www.facebook.com/cirgeo.unipd) or [Instagram](https://www.instagram.com/cirgeo.unipd/). 