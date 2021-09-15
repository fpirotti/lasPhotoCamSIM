// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2018 Intel Corporation


#ifndef LASHEMISPHERICSIM_HPP
#define LASHEMISPHERICSIM_HPP


 
#define AZIMUTHS   360 
#define ZENITHS    180  

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include  <stdint.h>
#include "lasreader.hpp" 
#include "laswriter.hpp"
// #include "tiff.h"
// #include "tiffio.h"
 
struct point
{
  double x;
  double y;
  double z;
};

struct arrIdx
{
  int row=-1;
  int col=-1; 
};

const double deg2rad(double x) {
  return(x * (M_PI/180.0)); 
}

const double rad2deg(double x) {
  return(x * (180.0/M_PI)); 
}

struct polarCoordinate
{
  double azimuth=0;
  double zenith=0;
  double distance=0;
};
// struct quantizer
// { 
//   int nAzimuths = AZIMUTHS;
//   int nZeniths = ZENITHS;
//   int hits[AZIMUTHS][ZENITHS];
// };
class quantizer{
  public: 
    int nAzimuths;
    int nZeniths;
    int nPlots;
    float ***domes; 
    float **images; 
    float maxvalue=.0;
    int mult=1;
    float zCam=1.3;
    float zenCut=89.0;
    bool cRaster=false;
    bool toLog=false;
    bool toDb=false;
    float weight=1.0;
    point *plotCenters;
    float *plotGapFraction; 
    char *basename;
    
    quantizer(int az, int ze, int plots, point *plotPositions, float zCam1=1.3, float zenCut1=89.0, 
              bool cRaster1=false, bool toLog1=false,  bool toDb1=false, 
              float weight1=1.0,  char *basename1=NULL, int mult=1) { 
      
      this->mult = mult;
      this->plotGapFraction = new float[plots];
      this->nAzimuths=az;
      this->nZeniths=ze;
      
      this->basename=strdup(basename1);
      
      this->cRaster=cRaster1;
      this->toLog=toLog1;
      this->toDb=toDb1;
      this->weight=weight1;
      
      nPlots=plots;
      this->zCam=zCam1;
      this->zenCut=zenCut1;
      plotCenters = plotPositions; 
      
      if(this->toDb && this->toLog){
        fprintf(stderr, "You cannot set both log10 and dB transformations! Only log10 will be applied.\n");
        this->toDb=false;
      }
      
      fprintf(stderr, "\n==============================\n"
                "Setup with plot center (1st plot) %.5f %.5f \n"
                "nZenith=%d  "
                "nAzimuths=%d  "
                "zCam=%.2f  "
                "zenCut=%.2f" 
                "\n", plotCenters[0].x, plotCenters[0].y,
                this->nZeniths,
                this->nAzimuths,
                this->zCam, 
                this->zenCut);
      
      this->domes = (float***)malloc(sizeof( *domes)  * plots);
      if(this->cRaster)  this->images = (float**)malloc(sizeof( *images) * plots);  
      // this->images[i] = (int*)malloc(sizeof(*this->images[i]) * nZeniths); 
      if (this->domes  )
      {
        int i;
        for (i = 0; i < plots; i++)
        {
          this->plotGapFraction[i]=0.0;
          this->domes[i] = (float**)malloc(sizeof(*this->domes[i])  * ZENITHS);   
          
          if(this->cRaster) {
            this->images[i] =  new float[(nZeniths*nZeniths*mult)];   
            // initialize nodata values -1 outside circle and zeros
            for(int iii=0; iii < (nZeniths*nZeniths*mult); iii++){
              int y=floor(iii/nZeniths) - nZeniths/2.0;
              int x=floor(iii%nZeniths) - nZeniths/2.0;
              float d=sqrt(y*y + x*x);
              if( d > nZeniths/2 ) this->images[i][iii] = -1.0f;
              else this->images[i][iii] = 0.0f;
              // 
              // fprintf(stderr,"<press %.f %d %d %.4f ENTER>\n", this->images[i][iii], x, y, d);
              // getc(stdin);
            }
          }
          
          if (this->domes[i] )
          {
            int j;
            for (j = 0; j < ZENITHS; j++)
            {
              this->domes[i][j] =  new float[AZIMUTHS];  
              memset( this->domes[i][j], 0, (AZIMUTHS)*sizeof(float) );
            }
          }
        }
      }
    }; 
     
     
    arrIdx polar2plane(double az, double zen, int gridWH=0, bool verbose=false) {  
       arrIdx ptProjected;
       if(gridWH==0){
         gridWH = nZeniths;
         if(verbose) fprintf(stderr, "WARNING: no size of projection grid, defaulting to %d;\n", nZeniths );
       } 
       
       double az1 = deg2rad( ((double)az - 180.0) );
       double zen1 = deg2rad( (double)zen );
       
       if(verbose && (az < 0 || az > 360.0 || zen < 0 || zen >= 90.0)  ) {
         fprintf(stderr, "WARNING: az=%.4f, z=%.4f\t==\trow=%d  col=%d\n", az, zen, ptProjected.row, ptProjected.col);
       }
       
       ptProjected.row = (int)(floor(( (sin(zen1)*cos(az1) * (double)gridWH) + (double)gridWH)/2.0 ) );
       ptProjected.col = (int)(floor(( (sin(zen1)*sin(az1) * (double)gridWH) + (double)gridWH)/2.0 ) );
       
       if(ptProjected.row==gridWH) ptProjected.row--;
       if(ptProjected.col==gridWH) ptProjected.col--;
       
       if( ptProjected.row < 0 || ptProjected.col < 0 || ptProjected.row > (gridWH-1) || ptProjected.col  > (gridWH-1) ) {
         fprintf(stderr, "WARNING: az=%.4f, z=%.4f\t==\trow=%d  col=%d\n", az, zen, ptProjected.row, ptProjected.col);
       }
       return(ptProjected);
     };
     
   void fillDomeGrid2(double az, double zen, double dist, int plotn=0){
     
     // counts for gap fraction are always 1°x1°
     
     int a= (int)(floor(az));
     if(a== nAzimuths) a--;
     // NB zenith is from 0 to 90 ( asin of positive values between 0 and 1) so we scale to nZeniths... e.g. if 180 nZeniths, zenith*2
     int z= (int)(floor(zen*2));
     if(z==nZeniths) z--;
     if ( this->domes[plotn][z][a] < (float)(INT32_MAX - 1)) {
       this->domes[plotn][z][a]=this->domes[plotn][z][a]+1.0/pow((dist+0.05),weight);
       // else  this->domes[plotn][z][a]=this->domes[plotn][z][a]+1.0;
     } 
     
     if(maxvalue < this->domes[plotn][z][a]) maxvalue = this->domes[plotn][z][a];
     
     if(this->cRaster){
       arrIdx idx;
       idx = polar2plane( az, zen, this->nZeniths );
       int fidx = nZeniths*idx.row+idx.col;
       
       if(this->weight && dist>1.0) images[plotn][fidx]=images[plotn][fidx]+1.0/pow((dist+0.05),weight);
       else images[plotn][fidx]=images[plotn][fidx]+1.0;
       
       // if(this->weight && dist>1.0) {fprintf(stderr, "ww==%.3f dist=%.3f www %.4f wwww %.4f\n", 
       //    this->weight, dist, images[plotn][fidx], 1.0/pow(dist,0.5) );
       //   
       //   fprintf(stderr,"<press ENTER>\n");
       //   getc(stdin);
       // }
       
     }
     
   }; 
   
   void fillDomeGrid(polarCoordinate p, int plotn=0 ){
     if(p.zenith > this->zenCut ) return;
     fillDomeGrid2( p.azimuth, p.zenith, p.distance,    plotn );
   }; 
   
   bool finalizePlotDome(int plotn,   bool verbose=false) {   
     int hits = 0; 
     // int imagea[(nZeniths*nZeniths)];
     // memset( imagea, 0, (nZeniths*nZeniths)*sizeof(int) );
      
     // USING ZENITS because it is fixed 180 360... 180 because zenit is 0.5°;
     for(int z=0; z <  ZENITHS; z++ ){
       for(int az=0; az <  AZIMUTHS; az++ ){
         if(this->domes[plotn][z][az] > 0) hits++; 
       }
     }
     
     this->plotGapFraction[plotn] = 100.0 - ((double)hits / ((double) AZIMUTHS*(ZENITHS-zenCut)) * 100.0) ;
     if(this->cRaster){
       dome2asc(plotn,  images[plotn] , verbose);
     }
     return(true); 
   };
   
   
   float* finalizePlotDomes(bool verbose=false) { 
     
      if(plotGapFraction==NULL){
        return(NULL); 
      }
      
      for(int i0=0; i0 <  nPlots; i0++ ){
        finalizePlotDome(i0,  verbose);
        if(verbose) fprintf(stderr, "%.2f\n", plotGapFraction[i0]);
      }
      return(plotGapFraction);
    };
   
 
 
 bool dome2asc(int plotn, float *image, bool verbose=false){
   
   char outfilenamet[1024];
   char outfilename[2048];
   // char outfilename2[1024];
   
   if(verbose) {
     fprintf(stderr, "Doing image for plot %d\n", plotn);
     if(this->toDb) fprintf(stderr, "Converting to dB ....\n"); 
     if(this->toLog) fprintf(stderr, "Converting to log10 ....\n");  
   }   
   
   // char ext[5] = ".asc";
   
   if(this->toDb){
    sprintf (outfilenamet,  "%s.plot%03d_dB", basename,  (plotn+1));
   } 
   else if(this->toLog){
     sprintf (outfilenamet,  "%s.plot%03d_log10", basename, (plotn+1));
   }
   else {
     sprintf (outfilenamet,  "%s.plot%03d", basename, (plotn+1));
   }
   
   if(this->weight!=0.0){
     sprintf(outfilename, "%s_w%.2f.asc", outfilenamet, this->weight);
   }  else {
     sprintf(outfilename, "%s.asc", outfilenamet);
   }
     
   // sprintf (outfilename2, "Plot_%03d.tfw", (plotn+1));
   
   FILE *out= fopen(outfilename, "w");
   
   fprintf(out, "ncols         %d \n"
             "nrows         %d \n"
             "xllcorner     %f\n"
             "yllcorner     %f\n"
             "cellsize      %f \n"
             "NODATA_value  -1\n", nZeniths, nZeniths, 
             (plotCenters[plotn].x - (float)nZeniths/( (float)nZeniths/90.0) ), 
             (plotCenters[plotn].y - (float)nZeniths/( (float)nZeniths/90.0) ),
             (180.0/(float)nZeniths) );
   
   for (int row = 0; row < nZeniths; row++)
   {  
     // memcpy(buf,  &image[nZeniths*row], linebytes);  
     for (int col = 0; col < nZeniths; col++){
       float v=((float)(image[nZeniths*row+col]));
       if(v < .0f){
         fprintf(out, "-1.0 "   ) ;
         continue;
       }
       if(this->toLog){
         fprintf(out, "%.6f " , log10((v+1.0)) ) ;
         continue;
        }
       
       if(this->toDb){
         fprintf(out, "%.6f " ,  -10.0*log10((v+1) / (maxvalue+1)) ) ;
         continue;
       }
      fprintf(out, "%.4f " , image[nZeniths*row+col]) ; 
     }
     fprintf(out, "\n"   ) ; 
    }
 
   if(ferror (out)) fprintf(stderr, "ERROR: File %s write error \n", outfilename); 
   
   fclose(out);  
   if(verbose) fprintf(stderr, "Written file %s with  image for plot %d\n", outfilename, plotn);  
    
   return(true);
   
 }
};

 
   
//    
//    bool dome2tiff(int plotn, int *image, bool verbose=false, bool tolog=false){
//       
//        char outfilename[1024];
//        char outfilename2[1024];
//         
//        if(verbose) fprintf(stderr, "Doing image for plot %d\n", plotn);
//        
//        sprintf (outfilename,  "Plot_%03d.tif", (plotn+1));
//        sprintf (outfilename2, "Plot_%03d.tfw", (plotn+1));
//        
//        TIFF *out= TIFFOpen(outfilename, "wb");
// 
//        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, nZeniths);  // set the width of the image
//        TIFFSetField(out, TIFFTAG_IMAGELENGTH, nZeniths);    // set the height of the image
//        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
//        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 32);    // set the size of the channels
//        TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
//        //   Some other essential fields to set that you do not have to understand for now.
//        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
//        
//        tsize_t linebytes = sizeof(int) * nZeniths;     
//        int *buf = NULL;   float *buff=NULL;     // buffer used to store the row of pixel information for writing to file
//        //    Allocating memory to store the pixels of current row
//        if (TIFFScanlineSize(out)==linebytes){
//          buf =(int *)_TIFFmalloc(linebytes);
//          buff =(float *)_TIFFmalloc(linebytes);
//        } else{
//          buf = (int *)_TIFFmalloc(TIFFScanlineSize(out));  
//          buff =(float *)_TIFFmalloc(TIFFScanlineSize(out));
//          fprintf(stderr, "WARNING 2  %ld  %ld  %ld\n",  TIFFScanlineSize(out), TIFFScanlineSize(out), linebytes ) ;
//        }
//        // We set the strip size of the file to be size of one row of pixels
//        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, linebytes));
//        // if(verbose) fprintf(stderr, "Do  %d\n",  image[11] ) ;
//        //Now writing image to the file one strip at a time
//        for (int row = 0; row < nZeniths; row++)
//        { 
//          // if(verbose) fprintf(stderr, "Do  %d\n",  image[row+10] ) ; 
//          memcpy(buf,  &image[nZeniths*row], linebytes); 
//          memcpy(buff, &image[nZeniths*row], linebytes); 
//          
//          // for (int roww = 0; roww < nZeniths; roww++) buff[roww] = log((double)(buff[roww]));
//          
//          if (TIFFWriteScanline(out, buff, row, 0) < 0){
//             if(verbose){ 
//               fprintf(stderr, "WARNING  %d\n",  row ) ;
//               fprintf(stderr,"<press ENTER>\n");
//               getc(stdin);
//               }
//            break; 
//            }
//        }
//         
//        TIFFClose(out); 
//        if (buf)
//          _TIFFfree(buf);
//        if (buff)
//          _TIFFfree(buff);
//        if(verbose) fprintf(stderr, "Done image for plot %d\n", plotn); 
//      
//      if(verbose) fprintf(stderr, "Doing image for plot %d\n", plotn);
//      return(true);
//      
//    }
// };


void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasHemisphericSIM -i in.las -loc plotPositions.csv -verbose  -zCam 1.3 -zenithCut 89 -orast -log \n"); 
  fprintf(stderr,"lasHemisphericSIM -h\n");
  fprintf(stderr,"-orast exports 180x180 pixel rasters in ESRI GRID ASCII format. Pixels represent the point counts. \n");
  fprintf(stderr,"-log converts pixel values, which represent point counts, to natural log scale (-orast must be also present). \n");
  fprintf(stderr,"-loc <file path> is the path to a CSV file with X Y coordinates - with header - other columns can be present and will be saved in output. Comma, tab, pipe, space, column and semi-column characters are accepted as column separators.\n");
  fprintf(stderr,"-zCam <height value in meters> default=1.3m \n\t- height of camera - NB this is in absolute height with respect to the point cloud, so if your point cloud is normalized (e.g. a canopy height model) then 1.3m will be 1.3m from the ground.  \n");
  fprintf(stderr,"-zenCut <Zenith angle in degrees> default=89° \n\t- At 90° the points will be at the horizon, potentially counting million of \n\tpoints: a smaller Zenith angle will ignore points lower than that angle \n\t(e.g. at 1km distance any point lower than 17.5 m height with respect to the camera ) .  \n");
  fprintf(stderr,"Output: the CSV file with an extra added column with Gap Fraction in percentage and, if '-orast' parameter is present, raster images 180x180 pixels in ESRI GRID ASCII format (https://en.wikipedia.org/wiki/Esri_grid).  \n");
  fprintf(stderr,"Version 0.9: for feedback contact author: Francesco Pirotti, francesco.pirotti@unipd.it  \n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}



 


void printPolar(polarCoordinate p){
  fprintf(stderr, "Polar = AZ: %.1f, Z=%.1f\n", 
          p.azimuth,  p.zenith);
}

void printPoint(LASpoint *point){
  fprintf(stderr, "pp %.8f, %.8f, %.2f\n", 
          point->coordinates[0], 
          point->coordinates[1], 
          point->coordinates[2]);
}

void original2plotCoords(LASpoint *pt, double x, double y) {  
  pt->coordinates[0] = pt->coordinates[0]-x;
  pt->coordinates[1] = pt->coordinates[1]-y;
}
void plotCoords2original(LASpoint *pt, double x, double y) {  
  pt->coordinates[0] = pt->coordinates[0]+x;
  pt->coordinates[1] = pt->coordinates[1]+y;
}

double distance3d(LASpoint *pt, double x=0, double y=0, double z=0, bool verbose=false) {  
  double dist = sqrt((pt->coordinates[0]-x)*(pt->coordinates[0]-x) + (pt->coordinates[1]-y)*(pt->coordinates[1]-y)  + (pt->coordinates[2]-z)*(pt->coordinates[2]-z) );
  
  if(verbose) fprintf(stderr, "distance  %.7f %.7f %.2f || %.7f  %.7f   %.2f  distance==%.7f ;\n",
     pt->coordinates[0],pt->coordinates[1], pt->coordinates[2], x, y, z, dist   );
  
  return( dist );
}

double distance2d(LASpoint *pt, double x=0, double y=0, bool verbose=false) {  
  if(verbose) fprintf(stderr, "distance  %.2f %.2f || %.2f  %.2f  distance==%.2f ;\n",
     pt->coordinates[0],pt->coordinates[1], x, y,
     sqrt((pt->coordinates[0]-x)*(pt->coordinates[0]-x) + (pt->coordinates[1]-y)*(pt->coordinates[1]-y) )  );
  
  return( sqrt((pt->coordinates[0]-x)*(pt->coordinates[0]-x) + (pt->coordinates[1]-y)*(pt->coordinates[1]-y) ) );
}

bool  isWithin2d(LASpoint *pt, double x, double y, double distLimit=1000) { 
  return(distance2d(pt, x, y)<distLimit);
}
bool  isWithin3d(LASpoint *pt, double x, double y, double z, double distLimit=1000) { 
  return(distance3d(pt, x, y, z)<distLimit);
}
bool  isWithin(double dist, double distLimit) { 
  return(dist<distLimit);
}
void  crt2str(LASpoint *pt) {  
  double sq = (sqrt(pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1] + pt->coordinates[2]*pt->coordinates[2]) + pt->coordinates[2]);
  pt->coordinates[0] = ( pt->coordinates[0]/sq);
  pt->coordinates[1] = ( pt->coordinates[1]/sq); 
}
 
void crt2eqd(LASpoint *pt) {  
  double sq = (sqrt(pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1])) * atan2(sqrt(pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1]),pt->coordinates[2]);
  pt->coordinates[0] =  (pt->coordinates[0]/sq);
  pt->coordinates[1] =  (pt->coordinates[1]/sq); 
}

// crtPlot coordinates referred to plot center (see original2plotCoords function)

polarCoordinate crtPlot2polar(LASpoint *pt) {  
  polarCoordinate pol;
  double dist = distance3d(pt);
  pol.distance = dist;
  if( (pt->coordinates[0]==pt->coordinates[1]) && (pt->coordinates[1]==0.0) ){
    pol.azimuth = 0;
    pol.zenith = 0;
  } else{ 
    pol.azimuth = rad2deg( atan2(pt->coordinates[1],pt->coordinates[0]) )+180.0;
    pol.zenith =  rad2deg( acos(pt->coordinates[2] / dist) );
  }
  return(pol);
}

 
void crt2ort(LASpoint *pt) {
  double sq = sqrt(pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1] + pt->coordinates[2]*pt->coordinates[2]);
  pt->coordinates[0] = ( pt->coordinates[0]/ sq );
  pt->coordinates[1] = ( pt->coordinates[1]/ sq );
 
}

void  crt2esa(LASpoint *pt) {
  double sq = sqrt( 2 * (pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1])) * sqrt(1 - (pt->coordinates[2]/sqrt(pt->coordinates[0]*pt->coordinates[0] + pt->coordinates[1]*pt->coordinates[1] + pt->coordinates[2]*pt->coordinates[2])));
  pt->coordinates[0] = ( pt->coordinates[0]/sq );
  pt->coordinates[1] = ( pt->coordinates[1]/sq );
 
}
// 
// static const char* getfield(char* line, int num, const char t[]=" ")
// {
//   const char* tok;
//   for (tok = strtok(line, t);
//        tok && *tok;
//        tok = strtok(NULL, "\n"))
//   {
//     if (!--num)
//       return tok;
//   }
//   return NULL;
// }


static void byebye(bool error=false, bool wait=false)
{
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(error);
}

static double taketime()
{
  return (double)(clock())/CLOCKS_PER_SEC;
}

#endif // LASHEMISPHERICSIM_HPP
