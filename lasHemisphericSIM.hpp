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
#include "tiff.h"
#include "tiffio.h"
 
struct point
{
  double x;
  double y;
  double z;
};

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
    int ***domes; 
    int ***images; 
    float *plotGapFraction; 
    
    quantizer(int az, int ze, int plots) { 
      
      this->plotGapFraction = new float[plots];
      this->nAzimuths=az;
      this->nZeniths=ze;
      nPlots=plots;
      this->domes = (int***)malloc(sizeof( *domes) * plots);  
      this->images = (int***)malloc(sizeof( *domes) * plots);  
      if (this->domes && this->images)
      {
        int i;
        for (i = 0; i < plots; i++)
        {
          this->plotGapFraction[i]=0.0;
          this->domes[i] = (int**)malloc(sizeof(*this->domes[i]) * nZeniths); // type of *arr[i] is T *
          this->images[i] = (int**)malloc(sizeof(*this->images[i]) * nZeniths * nZeniths); // type of *arr[i] is T *
          if (this->domes[i] && this->images[i])
          {
            int j;
            for (j = 0; j < nZeniths; j++)
            {
              this->domes[i][j] =  new int[nAzimuths]; //(int*)malloc(sizeof( *this->domes[i][j]) * nAzimuths);
              this->images[i][j] = new int[nZeniths]; // (int*)malloc(sizeof( *this->images[i][j]) * nZeniths);
            }
          }
        }
      }
    }; 
     

   void fillDomeGrid2(float az, float zen, int plotn=0){
     int a= (int)(floor(az));
     if(a== nAzimuths) a--;
     int z= (int)(floor(zen));
     if(z==nZeniths) z--;
     if ( this->domes[plotn][z][a] < (INT32_MAX - 1)) {
       this->domes[plotn][z][a]++;
     }
     
  }; 
   void fillDomeGrid(polarCoordinate p, int plotn=0){
     fillDomeGrid2( p.azimuth  , p.zenith ,   plotn );
   }; 
   
   bool finalizePlotDome(int plotn, bool createImage=false) {   
     int hits = 0; 
     int image[nZeniths][nZeniths]; 
     for(int i1=0; i1 <  nZeniths; i1++ ){
       for(int i2=0; i2 <  nAzimuths; i2++ ){
         if(this->domes[plotn][i1][i2] > 0) hits++;
         if(createImage){
           image[nZeniths][nZeniths]=ceil((image[nZeniths][nZeniths] + hits)/2);
         }
       }
     }
     this->plotGapFraction[plotn] = 100.0 - ((double)hits / ((double) this->nAzimuths*this->nZeniths) * 100.0) ;
     dome2tiff(plotn, *image); 
       
     return(true); 
   };
   
   void finalizePlotDomes(bool createImages=false, bool verbose=false) { 
     if(plotGapFraction==NULL){
      return; 
      }
      for(int i0=0; i0 <  nPlots; i0++ ){
        finalizePlotDome(i0, createImages);
        if(verbose) fprintf(stderr, "%.2f\t", plotGapFraction[i0]);
      } 
    };
   
 
   
   
   bool dome2tiff(int plotn, int *image, bool verbose=false){
     //for(int plotn=0; plotn <  nPlots; plotn++ ){
       char outfilename[1024];
       char outfilename2[1024];
       sprintf (outfilename, "Plot_%03d.tif", (plotn+1));
       sprintf (outfilename2, "Plot_%03d.tfw", (plotn+1));
       TIFF *out= TIFFOpen(outfilename, "w");
       TIFFSetField (out, TIFFTAG_IMAGEWIDTH, nZeniths);  // set the width of the image
       TIFFSetField(out, TIFFTAG_IMAGELENGTH, nZeniths);    // set the height of the image
       TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
       TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, sizeof(int));    // set the size of the channels
       TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
       //   Some other essential fields to set that you do not have to understand for now.
       TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
       TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
       
       tmsize_t linebytes = sizeof(int) * nZeniths;     
       int *buf = NULL;        // buffer used to store the row of pixel information for writing to file
       //    Allocating memory to store the pixels of current row
       if (TIFFScanlineSize(out)==linebytes)
         buf =(int *)_TIFFmalloc(linebytes);
       else
         buf = (int *)_TIFFmalloc(TIFFScanlineSize(out));
       
       // We set the strip size of the file to be size of one row of pixels
       TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, linebytes));
       
       //Now writing image to the file one strip at a time
       for (int row = 0; row < nAzimuths; row++)
       {
         //&this->images[plotn][nAzimuths]
         memcpy(buf, &image[nAzimuths], linebytes);    // check the index here, and figure out why not using h*linebytes
         if (TIFFWriteScanline(out, buf, row, 0) < 0)
           break;
       }
       
       (void) TIFFClose(out);
     //} 
     return(true);
     
   }
};


void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasHemisphericSIM -i in.las -loc locations.csv -verbose -orast raster/output/directory \n"); 
  fprintf(stderr,"lasHemisphericSIM -h\n");
  fprintf(stderr,"-loc <text file path> is the path to a CSV file with X Y coordinates - other columns are ok and will be save in output. Comma, tab, pipe, and semi-column characters are accepted as column separators.\n");
  fprintf(stderr,"Output: the CSV file with an extra added column with Gap Fraction in percentage and, if '-orast' parameter is present, raster representations.  \n");
  fprintf(stderr,"For feedback contact author: Francesco Pirotti, francesco.pirotti@unipd.it  \n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}

const double deg2rad(double x) {
  return(x * (M_PI/180.0)); 
}

const double rad2deg(double x) {
  return(x * (180.0/M_PI)); 
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
  if( (pt->coordinates[0]==pt->coordinates[1]) && (pt->coordinates[1]==0.0) ){
    pol.azimuth = 0;
    pol.zenith = 0;
  } else{ 
    pol.azimuth = rad2deg( atan2(pt->coordinates[0],pt->coordinates[1]) )+180.0;
    pol.zenith = rad2deg( asin(pt->coordinates[2] / distance3d(pt)) );
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
