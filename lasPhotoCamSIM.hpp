// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2018 Intel Corporation


#ifndef lasPhotoCamSIM_HPP
#define lasPhotoCamSIM_HPP


 
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
  double x=.0;
  double y=.0;
  double z=.0;
  bool isImage = false;
};

struct plotPoint
{
  double x=.0;
  double y=.0;
  double z=.0; 
  double *ori=NULL;
  // double yaw=.0;
  // double roll=.0;
  // double pitch=.0; 
  float zencut=90.0; 
  int orast=180; 
  int proj=0;
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
  double distance2d=0;
  point planar;
};



void camera2imageStr(polarCoordinate *pt, double c=1.0 ) {  
  double x, y;
  
  if(pt->distance2d==0) {x=.0; y=.0;} 
  else { 
    x = (tan(pt->zenith/2.0) * 2.0 * cos(pt->azimuth))/ (tan(M_PI/4.0)*2.0); 
    y = (tan(pt->zenith/2.0) * 2.0 * sin(pt->azimuth))/ (tan(M_PI/4.0)*2.0); 
  }
  
  pt->planar.x = x;  
  pt->planar.y = y;  
  pt->planar.isImage=true;   
}

void camera2imageEqd(polarCoordinate *pt, double c=1.0) {  
  double x, y;
   x =   (pt->zenith * cos(pt->azimuth))/(M_PI/2.0);   
   y =   (pt->zenith * sin(pt->azimuth))/(M_PI/2.0);   
  pt->planar.x = x;  
  pt->planar.y = y;  
  pt->planar.isImage=true;
}

void camera2imageEqa(polarCoordinate *pt, double c=1.0) {  
  double x, y;
  // x = sin(pt->zenith/2.0) * 2.0 * cos(pt->azimuth);  
  // y = sin(pt->zenith/2.0) * 2.0 * sin(pt->azimuth); 
  x = (sin(pt->zenith/2.0) * cos(pt->azimuth)) / sin(M_PI/4.0);  
  y = (sin(pt->zenith/2.0) * sin(pt->azimuth)) / sin(M_PI/4.0); 
  pt->planar.x = x;  
  pt->planar.y = y;  
  pt->planar.isImage=true;
  // fprintf(stderr, "\n ....%f %d", x, pt->planar.isImage);    
}

void camera2imageOrt(polarCoordinate *pt, double c=1.0) {  
  
  double x, y;
  x = (sin(pt->zenith)  * cos(pt->azimuth));  
  y = (sin(pt->zenith)  * sin(pt->azimuth));  
  
  pt->planar.x = x;  
  pt->planar.y = y;  
  pt->planar.isImage=true;   
}

void camera2imageRect(polarCoordinate *pt, double c=1.0, double fov=M_PI/4) {  
  double x, y;
  
  if(pt->distance2d==0) {x=.0; y=.0;} 
  // else if(pt->zenith > fov ) { 
  //   pt->planar.isImage=false;  
  //   } 
  // else  { 
    x = tan(pt->zenith)  * cos(pt->azimuth); 
    y = tan(pt->zenith)  * sin(pt->azimuth); 
    pt->planar.x = x;  
    pt->planar.y = y;  
    pt->planar.isImage=true; 
  // }
  
}

// NB requires that point coordinates are relative to camera space (original2cameracoords) and that polar coordinates are provided
void camera2image(polarCoordinate *pt, int projection, double c=1.0) {  
  
  if(projection==1) camera2imageStr(pt,c);
  else if(projection==2) camera2imageEqa(pt,c);
  else if(projection==3) camera2imageEqd(pt,c);
  else if(projection==4) camera2imageOrt(pt,c);
  else if(projection==5) camera2imageRect(pt,c);
  else {
    camera2imageEqa(pt,c);
  }
  // atan2()
  
  // pt->coordinates[0];
  // pt->coordinates[1];
  // pt->coordinates[2];
}


class quantizer{
  public:   
    float ***grids;  
    float maxvalue=.0; 
    int nPlots;
    double maxx = .0; 
    double maxy = .0;
    
    float zenCut=(M_PI/2.0);
    int orast=180;
    float radiusGrid=90.0f;
    bool toLog=false;
    bool toDb=false;
    float weight=1.0;
    plotPoint *plotCenters;
    float *plotGapFraction; 
    char *basename; 
    char projchar[6][4] = {"non", "str", "eqa", "eqd", "ort", "rct" };
    
    quantizer(  int plots, plotPoint *plotPositions,  
              float zenCut1=(M_PI/2.0), 
              bool toLog1=false,  bool toDb1=false, 
              float weight1=1.0,  char *basename1=NULL ) { 
       
      this->plotGapFraction = new float[plots];  
       
      // if(projchar1==NULL){
      //   this->projchar=strdup("eqa");
      //   fprintf(stderr, "Projection null!!!!!.\n");
      //   
      // } else {
      //   this->projchar =strdup(projchar1);  
      // }
      
      if(basename1==NULL){
        this->basename=strdup("lasPhotoCamSIMoutputCSV");
      } else {
        this->basename=strdup(basename1); 
      }
      
      this->zenCut=zenCut1;
      // this->plotCenters[plotn].orast=orast1;
      this->toLog=toLog1;
      this->toDb=toDb1;
      this->weight=weight1;
      
      this->nPlots=plots;
 
      plotCenters = plotPositions; 
      
      if(this->toDb && this->toLog){
        fprintf(stderr, "You cannot set both log10 and dB transformations! Only log10 will be applied.\n");
        this->toDb=false;
      }
      
      fprintf(stderr, "\n==============================\n"
                "Setup with:\n"
                "- plot center (1st plot) %.5f %.5f \n" 
                "- zenCut=%.2f (radians)\n" 
                "- zenCut=%.2f (degrees)\n" 
                "- grid size =%d\n" 
                "- prj =%s\n" 
                "\n", plotCenters[0].x, plotCenters[0].y, 
                this->zenCut, 
                rad2deg(this->zenCut),
                plotCenters[0].orast,
                projchar[ plotCenters[0].proj ]);
      
      this->grids = (float***)malloc(sizeof( *grids)  * plots);

      if (this->grids  )
      {
        int i;
        for (i = 0; i < plots; i++)
        {
          this->plotGapFraction[i]=0.0;
          this->grids[i] = (float**)malloc(sizeof(*this->grids[i])  * this->plotCenters[i].orast );   
           
           
          radiusGrid = (this->plotCenters[i].orast / 2.0) * sin(this->zenCut) ;
          if (this->grids[i] )
          { 
            for (int j = 0; j < this->plotCenters[i].orast; j++)
            {
              this->grids[i][j] =  new float[this->plotCenters[i].orast];  
              memset( this->grids[i][j], 0, (this->plotCenters[i].orast)*sizeof(float) );
              
              for(int k=0; k < this->plotCenters[i].orast; k++){
                if(this->plotCenters[i].proj!=5 && sqrt(( pow((j - radiusGrid),2.0)  + pow((k - radiusGrid),2.0)  )) > radiusGrid ) this->grids[i][j][k]= -1.0f;
                else this->grids[i][j][k]=0.0f;
              }
            }
          }
        }
      }
    }; 
      
      void image2grid(int plotn, polarCoordinate *pt,    bool verbose=false) {
        
        if(!pt->planar.isImage) return;
        int x, y;
        x = (int)floor(((double)this->plotCenters[plotn].orast * pt->planar.x + (double)this->plotCenters[plotn].orast)/2.0);
        y = (int)floor(((double)this->plotCenters[plotn].orast * pt->planar.y + (double)this->plotCenters[plotn].orast)/2.0);
        // if(x > (this->plotCenters[plotn].orast-1)) x--;
        // if(y==this->plotCenters[plotn].orast) y--;
        
        if(x < 0 || x >= this->plotCenters[plotn].orast || y < 0 || y >= this->plotCenters[plotn].orast ){
          if(verbose)        fprintf(stderr, "\n proj=%s \n az=%f zen=%f ------- %f  \t  \n%.2f  %f \n%.2f  %f -- \n"
                                       "x=%d\ty=%d \n"
                                       "%f \n"  ,
                                       projchar[ this->plotCenters[plotn].proj ], pt->azimuth, pt->zenith,
                                       (double)this->plotCenters[plotn].orast,
                                       this->plotCenters[plotn].orast * pt->planar.x,
                                       this->plotCenters[plotn].orast * pt->planar.y,
                                       pt->planar.x,
                                       pt->planar.y,
                                       x, y, ((double)this->plotCenters[plotn].orast * pt->planar.y + (double)this->plotCenters[plotn].orast) );
          
        } else {
          
          this->grids[plotn][y][x] = this->grids[plotn][y][x] + (1.0/pow((pt->distance+0.05),weight));
        }
        
      }
   
  
   float* finalizePlotDomes(bool verbose=false) {

      if(plotGapFraction==NULL){
        return(NULL);
      }

      for(int i0=0; i0 <  nPlots; i0++ ){
        dome2asc(i0,  grids[i0] , verbose);
        if(verbose) fprintf(stderr, "%.2f\n", plotGapFraction[i0]);
      }
      return(plotGapFraction);
    }; 
 
 bool dome2asc(int plotn, float **image, bool verbose=false){
   
   char outfilenamet[1024];
   char outfilename[2048];
   
    
   
   if(verbose) {
     fprintf(stderr, "Doing image for plot %d\n", plotn);
     if(this->toDb) fprintf(stderr, "Converting to dB ....\n"); 
     if(this->toLog) fprintf(stderr, "Converting to log10 ....\n");  
   }   
   
   
   if(this->toDb){
    sprintf (outfilenamet,  "%s.plot%03d_%s_zenCut%d_sz%d_dB", basename,  (plotn+1), projchar[ this->plotCenters[plotn].proj ], (int)(rad2deg(zenCut)), this->plotCenters[plotn].orast );
   } 
   else if(this->toLog){
     sprintf (outfilenamet,  "%s.plot%03d_%s_zenCut%d_sz%d_log10", basename, (plotn+1), projchar[  this->plotCenters[plotn].proj ], (int)(rad2deg(zenCut)), this->plotCenters[plotn].orast );
   }
   else {
     sprintf (outfilenamet,  "%s.plot%03d_%s_zenCut%d_sz%d", basename, (plotn+1), projchar[  this->plotCenters[plotn].proj ], (int)(rad2deg(zenCut)), this->plotCenters[plotn].orast );
   }
   
   if(this->weight!=0.0){
     sprintf(outfilename, "%s_w%.2f.asc", outfilenamet, this->weight);
   }  else {
     sprintf(outfilename, "%s.asc", outfilenamet);
   }
     
   if(verbose) {
     fprintf(stderr, "Doing image for plot %d\nFilename=%s\n", plotn, outfilename);
     if(this->toDb) fprintf(stderr, "Converting to dB ....\n");
     if(this->toLog) fprintf(stderr, "Converting to log10 ....\n");
   }
   FILE *out= fopen(outfilename, "w");
   
   fprintf(out, "ncols         %d \n"
             "nrows         %d \n"
             "xllcorner     %f\n"
             "yllcorner     %f\n"
             "cellsize      %f \n"
             "NODATA_value  -1\n", this->plotCenters[plotn].orast , this->plotCenters[plotn].orast, 
             (plotCenters[plotn].x - (float)this->plotCenters[plotn].orast/( (float)this->plotCenters[plotn].orast/90.0) ), 
             (plotCenters[plotn].y - (float)this->plotCenters[plotn].orast/( (float)this->plotCenters[plotn].orast/90.0) ),
             (180.0/(float)this->plotCenters[plotn].orast) );
   
   
   

   int hits=0;
   int total=0;
   for (int row = 0; row < this->plotCenters[plotn].orast; row++)
   {   
     for (int col = 0; col < this->plotCenters[plotn].orast; col++){
       
       float v=((float)(image[row][col])) * this->plotCenters[plotn].orast; 
        
       // fprintf(stderr, "\n ------- %.2f   "  , v);
       
       if(v < .0f){
         fprintf(out, "-1.0 "   ) ;
         continue;
       }
       total++;
       if(v > .0f) hits++; 
       
       if(this->toLog){
         fprintf(out, "%.6f " , log10((v+1.0)) ) ;
         continue;
        }
       
       if(this->toDb){
         fprintf(out, "%.6f " ,  -10.0*log10((v+1) / (maxvalue+1)) ) ;
         continue;
       }
      fprintf(out, "%.2f " , v) ; 
     }
     fprintf(out, "\n"   ) ; 
    }
 
   if(ferror (out)) fprintf(stderr, "ERROR: File %s write error \n", outfilename); 
   
   fclose(out);  
   if(verbose) fprintf(stderr, "Written file %s with  image for plot %d\n", outfilename, plotn);  
    
   
   this->plotGapFraction[plotn] = 100.0 - ((double)hits / ((double) total) * 100.0) ;
    
   return(true);
   
 }
};




 


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

void rotate(LASpoint *pt, double *ori) {
  double cosa = cos(ori[2]);
  double sina = sin(ori[2]);
  
  double cosb = cos(ori[1]);
  double sinb = sin(ori[1]);
  
  double cosc = cos(ori[0]);
  double sinc = sin(ori[0]);
  
  double Axx = cosa*cosb;
  double Axy = cosa*sinb*sinc - sina*cosc;
  double Axz = cosa*sinb*cosc + sina*sinc;
  
  double Ayx = sina*cosb;
  double Ayy = sina*sinb*sinc + cosa*cosc;
  double Ayz = sina*sinb*cosc - cosa*sinc;
  
  double Azx = -sinb;
  double Azy = cosb*sinc;
  double Azz = cosb*cosc;
  
    double px = pt->coordinates[0];
    double py = pt->coordinates[1];
    double pz = pt->coordinates[2];
  
  // fprintf(stderr, "rotat %.1f %.1f %.1f \n", 
  //         pt->coordinates[0], 
  //                        pt->coordinates[1], 
  //                                       pt->coordinates[2]); 
  
    pt->coordinates[0] = Axx*px + Axy*py + Axz*pz;
    pt->coordinates[1] = Ayx*px + Ayy*py + Ayz*pz;
    pt->coordinates[2] = Azx*px + Azy*py + Azz*pz;
    
    // fprintf(stderr, " to  %.1f %.1f %.1f \n\n", 
    //         pt->coordinates[0], 
    //         pt->coordinates[1], 
    //         pt->coordinates[2]); 
}

void original2cameracoords(LASpoint *pt, plotPoint camera) {  
  pt->coordinates[0] = pt->coordinates[0]- camera.x;
  pt->coordinates[1] = pt->coordinates[1]- camera.y;
  pt->coordinates[2] = pt->coordinates[2]- camera.z;
  if(camera.ori==NULL) return;
  rotate(pt, camera.ori);
  
}

void cameraCoords2original(LASpoint *pt, double x, double y, double z) {  
  pt->coordinates[0] = pt->coordinates[0]+x;
  pt->coordinates[1] = pt->coordinates[1]+y;
  pt->coordinates[2] = pt->coordinates[2]+z;
}

 
double distance3d(LASpoint *pt, double x=0, double y=0, double z=0, bool verbose=false) {  
  double dist = sqrt((pt->coordinates[0]-x)*(pt->coordinates[0]-x) + (pt->coordinates[1]-y)*(pt->coordinates[1]-y)  + (pt->coordinates[2]-z)*(pt->coordinates[2]-z) );
  
  if(verbose) fprintf(stderr, "distance  %.7f %.7f %.2f || %.7f  %.7f   %.2f  distance==%.7f ;\n",
     pt->coordinates[0],pt->coordinates[1], pt->coordinates[2], x, y, z, dist   );
  
  return( dist );
}

double distance2d(LASpoint *pt, double x=0, double y=0, bool verbose=false) {  
  // if(verbose) fprintf(stderr, "distance  %.2f %.2f || %.2f  %.2f  distance==%.2f ;\n",
  //    pt->coordinates[0],pt->coordinates[1], x, y,
  //    sqrt((pt->coordinates[0]-x)*(pt->coordinates[0]-x) + (pt->coordinates[1]-y)*(pt->coordinates[1]-y) )  );
  // 
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

polarCoordinate  crtPlot2polar(LASpoint *pt) {  
  
  polarCoordinate pol;
  pol.distance = distance3d(pt);
  pol.distance2d = distance2d(pt);
  pol.planar.x = pt->coordinates[0];
  pol.planar.y = pt->coordinates[1];
  pol.planar.z = pt->coordinates[2];
  
  if( (pt->coordinates[0]==pt->coordinates[1]) && (pt->coordinates[1]==0.0) ){
    pol.azimuth = 0;
    pol.zenith = 0;
  } else{ 
    pol.azimuth = ( atan2(pt->coordinates[1],pt->coordinates[0]) );
    pol.zenith =  ( acos(pt->coordinates[2] / pol.distance) );
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

#endif // lasPhotoCamSIM_HPP
