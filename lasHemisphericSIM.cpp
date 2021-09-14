/*
 ===============================================================================
 
 FILE:  lasHemisphericSIM.cpp
 
 CONTENTS:
 
 This source code serves as an example how you can easily use LASlib to
 write your own processing tools or how to import from and export to the
 LAS format or - its compressed, but identical twin - the LAZ format.
 
 PROGRAMMERS:
 
 francesco.pirotti@unipd.it  -  https://www.cirgeo.unipd.it
 
 COPYRIGHT:
 
 (c) 2007-2014,  - simulate hemispheric photography in forest lidar
 
 This is free software; you can redistribute and/or modify it under the
 terms of the GNU Lesser General Licence as published by the Free Software
 Foundation. See the LICENSE.txt file for more information.
 
 This software is distributed WITHOUT ANY WARRANTY and without even the
 implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 CHANGE HISTORY:
 
 3 september 2021 -- created to compare with real hemispheric photography
 
 ===============================================================================
 */

#include "lasHemisphericSIM.hpp"



int main(int argc, char *argv[])
{
  int i;
  bool verbose = false;
  double start_time = 0.0;
  
  // int dbz[ZENITH][AZIMUTHS];
  
  LASreadOpener lasreadopener;
  FILE*  fpLocations;
  // FILE*  fpOutput;
  char file_name_location[256];
  double x[100];
  double y[100];
  int nPositions=0;
  //LASwriteOpener //laswriteopener;
  
  if (argc == 1)
  {
    fprintf(stderr,"%s is better run in the command line\n", argv[0]);
    char file_name[256];
    fprintf(stderr,"enter input file: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    lasreadopener.set_file_name(file_name);
    fprintf(stderr,"enter file with locations table (x y name ...): "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    //laswriteopener.set_file_name(file_name);
  }
  else
  {
    lasreadopener.parse(argc, argv);
    //laswriteopener.parse(argc, argv);
  }
  
  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      usage();
    }
    else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"-verbose") == 0)
    {
      verbose = true;
    } 
    else if (strcmp(argv[i],"-loc") == 0)
    { 
      
      i++;
      strcpy(file_name_location, argv[i]);
    } 
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      usage();
    }
    
  }
  
  if ( strlen(file_name_location) == 0){ 
    fprintf(stderr, "ERROR: Must include path to a text file with table with coordinates!\n");
    usage();
  }
  
  if (verbose) start_time = taketime();
  
  
  fpLocations = fopen(file_name_location, "r"); 
  
  // fail if output file does not open
  
  if (fpLocations == 0)
  {
    fprintf(stderr, "ERROR: could not open '%s' for read\n", file_name_location); 
    byebye(true, argc==1);
    
  } else {
    char line[2048];
    char *tmptokens[1024];
    fprintf(stderr, "reading '%s'  \n", file_name_location); 
    while (fgets(line, 1024, fpLocations))
    { 
      char *token, *str, *tofree, *fsep;
      line[strcspn(line, "\r\n")] = 0;
      tofree = str = strdup((char*)(&line));  // We own str's memory now. 
      if(strlen(strsep(&str, "\t"))!=strlen(str) ) fsep = strdup("\t");
      int tok=0;
      while ((token = strsep(&str, "\t"))) {
        (tmptokens[tok])=strdup(token);  
        tok++;
        if(tok==20){
          fprintf(stderr, "ERROR: up to 20 columns supported, your table seems to have more!\n"); 
          byebye(true, argc==1);
        }
      }
      
      if(tok<2){
        fprintf(stderr, "ERROR: at least 2 columns required (X and Y coordinates), without header line: your table seems to have only one. Check the delimiter, comma, tab, pipe, and semi-column characters are accepted as column separators. !\n"); 
        byebye(true, argc==1);
      } 
      x[nPositions] =  atof( tmptokens[0] );  
      y[nPositions] =  atof( tmptokens[1] );  
      
      free(tofree);
        
      if(nPositions>99){
        fprintf(stderr, "ERROR: More than 100 rows with locations, please split your table with plot location coordinates in less than 100 plots per file.\n"); 
        byebye(true, argc==1); 
      }
      
      nPositions++; 
       
    }
    if(verbose)  fprintf(stderr, "Read %d plot positions...\n", nPositions);
  }
  
  if(verbose) fprintf(stderr, "Finished reading '%s'  \n", file_name_location); 
  if (!lasreadopener.active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    usage(argc == 1);
  }
  
  
  quantizer *collector = new quantizer(AZIMUTHS,  ZENITHS, nPositions, 2); 
  
  // if(verbose) fprintf(stderr,"Reading %d LAS/LAZ files sampled on %d plots\n", nPositions); 
  
  while (lasreadopener.active()) {
    
    LASreader* lasreader = lasreadopener.open();
    
    #ifdef _WIN32
        if (verbose) fprintf(stderr, "...reading %I64d points from '%s' and writing gap-fraction  to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(),  file_name_location);
    #else
        if (verbose) fprintf(stderr, "...reading %lld points from '%s' and writing  gap-fraction to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), file_name_location );
    #endif
        
    if (lasreader == 0)
    {
      fprintf(stderr, "ERROR: could not open lasreader\n");
      byebye(argc==1);
    }
    
    int q = 100;
    if( lasreader->npoints > 100000000 ) q=1000;
    // loop over points and modify them
    int progress = lasreader->npoints/q;

    // fprintf(stderr,"<press ENTER %d -- >\n", collector->nPlots );
    // getc(stdin);
     
    while (lasreader->read_point())
    {   
      if(lasreader->point.get_z() < 1){
        continue;  
      }
      
      for(int i=0; i<nPositions; ++i){
        // grab coordinates 
         
        lasreader->point.compute_coordinates();
        // convert to plot center reference
        original2plotCoords(&lasreader->point, x[i], y[i]); 
        collector->fillDomeGrid( crtPlot2polar(&lasreader->point), i, true);
      
      } 
      
      if ( progress && ((lasreader->p_count % progress) == 0))
      {
        if(q==1000) fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b%03d / 1000",  (int) ((float)lasreader->p_count/(float)lasreader->npoints*1000.0)) ;
        else  fprintf(stderr, "\b\b\b%02d%%",  (int) ((float)lasreader->p_count/(float)lasreader->npoints*q)) ;
      }
    } 
     
    #ifdef _WIN32
      if (verbose) fprintf(stderr,"total time: %g sec   for %I64d points\n", taketime()-start_time,   lasreader->p_count);
    #else
      if (verbose) fprintf(stderr,"total time: %g sec   for %lld points\n", taketime()-start_time, lasreader->p_count);
    #endif
    
    lasreader->close();
    delete lasreader;
  }
  
  collector->finalizePlotDomes(true, true);  
  fclose(fpLocations);
   
  return 0;
}
