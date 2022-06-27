/*
 ===============================================================================

 FILE:  lasPhotoCamSIM.cpp

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

#include "lasPhotoCamSIM.hpp"

int countLines(char *fpname)
{
  FILE                *fp = fopen(fpname, "r");    /* or use fopen to open a file */
  int                 c;              /* Nb. int (not char) for the EOF */
  unsigned long       newline_count = 0;

  /* count the newline characters */
  while ( (c=fgetc(fp)) != EOF ) {
    if ( c == '\n' )
      newline_count++;
  }
  fclose(fp);
  return(newline_count);
}

void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasPhotoCamSIM -i in.las -loc cameraXYZpositions.csv -verbose  -zCam 0.0 -zenithCut 89 -orast 180 -log \n");
  fprintf(stderr,"lasPhotoCamSIM -h\n\nPARAMETERS:\n");

  fprintf(stderr,"-orast <size of square in pizels> default=180 \n\t"
            "- exports a square grid in ESRI GRID ASCII format. Pixels "
            "represent the point counts. Size of grid  \n\n");

  fprintf(stderr,"-proj <eqa|eqd|str|ort|rct> - lens projections:"
            " \n\tequisolid(equal area), \n\tequidistant, \n\tstereographic,"
            " \n\torthographic and \n\trectilinear (standard perspsective)\n\n"  );
  fprintf(stderr,"-ori <0.0 180.0 0.0> \n\t- camera orientation, three angles, respectively "
            "pitch, yaw and roll/tilt angles in degrees. It is an optional input."
            "If not set, it implies an upward looking camera (pitch=0.0). "
            "pitch min=0 max=180 (0 is upwards, 90 is horizon, 180 is downwards); \n"
            "yaw min=0 max=360; (0 is north, 90 is east, 180 is south) \n"
            "roll/tilt min=-180 max 180 (0 is levelled, negative values is rotation counterclockwise)\n"
            "E.g. 90.0 180.0 0.0 means a camera oriented towards the horizon looking south, not tilted. \n\n"
            );

  fprintf(stderr,"-weight <power value>: default=0.0 power of inverse distance weight. "
  "Each point that intersects a 1°x1° sector in the dome will be positioned"
  " at a certain distance that can be used to weight the value of the point. "
  "No weight=each point weights 1, i.e. if 10 points are in the sector, "
  "that sector will be occluded by 10 points. If weight=2 (-weight 2.0) is"
  " provided, each point will add a value of 1.0 * 1.0/pow(distance, 2.0) to the total.  "
  "Default value is 0 because no weight is applied\n\n");

  fprintf(stderr,"-maxdist <distance in meters> default=1000.0 \n\t- will ignore any points that"
            " are further than this value from the center of the camera. \n\n");
  fprintf(stderr,"-log - converts pixel values, which represent point counts,"
            " to natural log scale (-orast must be also present). \n\n");
  fprintf(stderr,"-loc <file path> \n\t- is the path to a CSV file with X Y and Z coordinates"
            " - with header - other columns can be present and will be saved in output."
            " Comma, tab, pipe, space, column and semi-column characters are accepted"
            " as column separators.\n\n");
  fprintf(stderr,"-zCam <height value in meters> default=0.0m \n\t- "
            "height of camera - NB this is in absolute height with respect to the"
            " point cloud, so if your point cloud is normalized (e.g. a canopy height model)"
            " then 1.3m will be 1.3m from the ground.  \n\n");
  fprintf(stderr,"-zenCut <Zenith angle in degrees> default=89° \n\t- "
            "At 90° the points will be at the horizon, potentially counting million of"
            " \n\tpoints: a smaller Zenith angle will ignore points lower than that angle.\n\n");
  fprintf(stderr,"Output: the CSV file with an extra added column with Gap Fraction in percentage "
            "and, if '-orast' parameter is present, raster images 180x180 pixels in ESRI GRID ASCII"
            " format (https://en.wikipedia.org/wiki/Esri_grid).  \n\n");
  fprintf(stderr,"Version 0.95.2: for feedback contact author: Francesco Pirotti, francesco.pirotti@unipd.it  \n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}


int main(int argc, char *argv[])
{
  int i;
  int plotN=0;
  bool verbose = false;
  double start_time = 0.0;

  // int dbz[ZENITH][AZIMUTHS];

  LASreadOpener lasreadopener;
  FILE*  fpLocations;
  // FILE*  fpOutput;
  char file_name_location[256];
  float zCam=.0f;
  float zenCut=deg2rad(89.9);
  float maxdist=1000.0;  // maximum distance to consider when counting points... 1 km
  int orast=180;
  double *ori=NULL;
  bool toLog=false;
  bool toDb=false;
  float weight=0.0;
  int proj=2;
  // char *projchar=strdup("eqa");
  plotPoint *plotPositions=NULL;
  int nPositions=0;
  char *field[1024];
  int csvProjIndex =-1;
  int csvYawIndex  =-1;
  int csvPitchIndex=-1;
  int csvRollIndex =-1;
  int csvOrastIndex=-1;
  int csvXIndex=-1;
  int csvYIndex=-1;
  int csvZIndex=-1;

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
    else if (strcmp(argv[i],"-ori") == 0)
    {
      ori = new double[3];
      i++;
      ori[0]=atof(argv[i]);
      if(ori[0]< -90.0||ori[0]>90.0) {
        fprintf(stderr, "ERROR:  picth angle inserted is '%s'"
                  " ONLY VALUES BETWEEN -90.0 AND 90.0 are allowed."
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);
      } ;
      if(ori[0]==0.0) {
        fprintf(stderr, "WARNING:  pitch angle inserted is '%s'"
                  " this means that it will be  looking at the horizon."
                  " - please check \n",
                  argv[i]);
      }

      i++;
      ori[1]=atof(argv[i]);

      if(ori[1]==0.0) {
        fprintf(stderr, "WARNING:  yaw angle inserted is '%s'."
                  " - please check \n",
                  argv[i]);
      }
      if(ori[1]<0.0||ori[1]>360.0) {
        fprintf(stderr, "ERROR:  yaw angle inserted is '%s'"
                  " ONLY VALUES BETWEEN 0 AND 360 are allowed."
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);
      }

      i++;
      ori[2]=atof(argv[i]);

      if(ori[2]==0.0) {
        fprintf(stderr, "WARNING:  roll angle inserted is '%s'."
                  " - please check \n",
                  argv[i]);
      }
      if(ori[2]<-360.0||ori[2]>360.0) {
        fprintf(stderr, "ERROR:  roll angle inserted is '%s'"
                  " ONLY VALUES BETWEEN 0 AND 360 are allowed."
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);
      }

      for(int i; i<3; i++){
        ori[i]=deg2rad(ori[i]);
      }
    }
    else if (strcmp(argv[i],"-orast") == 0 )
    {
      i++;
      orast=atoi(argv[i]);
      if(orast==0.0) {
        fprintf(stderr, "ERROR:  argument -orast '%s'"
                  " was converted to 0 which is not possible as it represents the width/height in pixels of the output square grid."
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);
      } ;
    }
    else if (strcmp(argv[i],"-log") == 0 )
    {
      toLog=true;
    }
    else if (strcmp(argv[i],"-db") == 0 )
    {
      toDb=true;
    }
    else if (strcmp(argv[i],"-proj") == 0 )
    {
      i++;
      if(strcmp(argv[i],"str")==0){
        proj=1;
      } else if(strcmp(argv[i],"eqa")==0){
        proj=2;
      } else if(strcmp(argv[i],"eqd")==0){
        proj=3;
      } else if(strcmp(argv[i],"ort")==0){
        proj=4;
      } else if(strcmp(argv[i],"rct")==0){
        proj=5;
      } else {
        fprintf(stderr, "ERROR:  argument -proj '%s'"
                  " not correct. Possible values are: \n\teqa, eqd, str, ort  \n\trespectively"
                  " for : \n\tequisolid/equiareal, equidistant, stereographic and orthographic projections"
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);

        }

      // projchar = strdup(argv[i]);

    }
    else if (strcmp(argv[i],"-maxdist") == 0 )
    {
      i++;
      maxdist=atof(argv[i]);
      if(maxdist==0.0) {
        fprintf(stderr, "ERROR:  argument -maxdist '%s'"
                  " was converted to 0 which is not possible"
                  " - please check \n",
                  argv[i]);
        byebye(true, argc==1);
      }
    }
    else if (strcmp(argv[i],"-weight") == 0 )
    {
      i++;
      weight=atof(argv[i]);
      if(weight==0.0) {
        fprintf(stderr, "ERROR:  argument -weight '%s'"
                  " was converted to 0.0 which is not possible"
                  " - please check \n",
                argv[i]);
        byebye(true, argc==1);
      }
    }
    else if (strcmp(argv[i],"-zenCut") == 0)
    {
      i++;
      zenCut=atof(argv[i]);
      if(zenCut==0.0 ) {
        fprintf(stderr, "WARNING:  argument -zenCut '%s' was converted to 0.0 zenith angle which means no field of view for camera (zenith angle 90° == horizon) - please check \n",
                argv[i]);
      }
      if(zenCut>90.0 || zenCut<0.0) {
        fprintf(stderr, "ERROR:  argument -zenCut '%s' was not converted to values -- 0.0 < angle < 90.0 -- zenith angle which does not make sense (zenith angle 90 degrees == horizon, 0 degrees = upward) - please check \n",
                argv[i]);
        byebye(true, argc==1);
      }
      zenCut = deg2rad(zenCut);
    }
    else if (strcmp(argv[i],"-zCam") == 0)
    {

      i++;
      zCam=atof(argv[i]);
      if(zCam <  0.0) {
        fprintf(stderr, "WARNING:  argument '-zCam %s' was converted to  a camera height lower than 0.0 m  - please check if this is what you want.\n",
                argv[i]);
      }
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



  nPositions = countLines(file_name_location);
  if (verbose) fprintf(stderr,"Number of plots: %d\n", nPositions);
  plotPositions = new plotPoint[nPositions];

  fpLocations = fopen(file_name_location, "r");

  // fail if output file does not open


  char  *fsep;

  if (fpLocations == 0)
  {
    fprintf(stderr, "ERROR: could not open '%s' for read\n", file_name_location);
    byebye(true, argc==1);

  } else {
    char line[2048];
    char *tmptokens[1024];
    char  *token, *str, *tofree ;

    // read first line with header
    fgets(line, 1024, fpLocations);

    line[strcspn(line, "\r\n")] = 0;

    fprintf(stderr, "Testing line '%s'\n", line);
    tofree = str = strdup((char*)(&line));
    fsep = strdup("\t");
    token = strtok(str, fsep);
    if(verbose) {
      fprintf(stderr, "Testing separator '%s'\n", fsep);
    }

    if(strlen(token)==strlen(line) ) {
      tofree = str = strdup((char*)(&line));
      fsep = strdup("|");
      if(verbose) {
        fprintf(stderr, "Testing separator '%s'\n", fsep);
      }
      token = strtok(str, fsep);
    }

    if(strlen(token)==strlen(line) ) {
      tofree = str = strdup((char*)(&line));
      fsep = strdup(";");
      if(verbose) {
        fprintf(stderr, "Testing separator '%s'\n", fsep);
      }
      token = strtok(str, fsep);
    }

    if(strlen(token)==strlen(line) ) {
      tofree = str = strdup((char*)(&line));
      fsep = strdup(":");
      if(verbose) {
        fprintf(stderr, "Testing separator '%s'\n", fsep);
      }
      token = strtok(str, fsep);
    }

    if(strlen(token)==strlen(line) ) {
      tofree = str = strdup((char*)(&line));
      fsep = strdup(",");
      if(verbose) {
        fprintf(stderr, "Testing separator '%s'\n", fsep);
      }
      token = strtok(str, fsep);
    }

    if(strlen(token)==strlen(line) ) {
      tofree = str = strdup((char*)(&line));
      fsep = strdup(" ");
      if(verbose) {
        fprintf(stderr, "Testing separator ' ' (space)\n");
      }
      token = strtok(str, fsep);
    }
    if(strlen(token)==strlen(line) )
    {
      fprintf(stderr, "ERROR: could not find separator of columns! Tested header '%s'. Please check https://github.com/fpirotti/lasPhotoCamSIM/ \n", line);
      byebye(true, argc==1);
    }


    int tok=0;
    tofree = str = strdup((char*)(&line));
    token = strtok(str, fsep);
    while (token) {
      (field[tok])=strdup(token);

      if(verbose) {
        fprintf(stderr, "Found column '%s' with separator '%s'\n", field[tok], fsep);
      }
      if(strcmp(field[tok], "X")==0 || strcmp(field[tok], "x")==0 || strcmp(field[tok], "\"X\"")==0 || strcmp(field[tok], "\"x\"")==0) csvXIndex=tok;
      if(strcmp(field[tok], "Y")==0 || strcmp(field[tok], "y")==0 || strcmp(field[tok], "\"Y\"")==0 || strcmp(field[tok], "\"y\"")==0) csvYIndex=tok;
      if(strcmp(field[tok], "Z")==0 || strcmp(field[tok], "z")==0 || strcmp(field[tok], "\"Z\"")==0 || strcmp(field[tok], "\"z\"")==0) csvZIndex=tok;
      if(strcmp(field[tok], "proj")==0 || strcmp(field[tok], "\"proj\"")==0) csvProjIndex=tok;
      if(strcmp(field[tok], "orast")==0 || strcmp(field[tok], "\"orast\"")==0) csvOrastIndex=tok;
      if(strcmp(field[tok], "yaw")==0 || strcmp(field[tok], "\"yaw\"")==0 ){
        csvYawIndex=tok;
      }
      if(strcmp(field[tok], "pitch")==0 || strcmp(field[tok], "\"pitch\"")==0  ){
        csvPitchIndex=tok;
      }
      if(strcmp(field[tok], "roll")==0 || strcmp(field[tok], "\"roll\"")==0   ){
        csvRollIndex=tok;
      }

      tok++;
      if(tok==50){
        fprintf(stderr, "ERROR: up to 50 columns supported, your table seems to have more!\n");
        byebye(true, argc==1);
      }
      token = strtok(NULL, fsep);
    }
    if(tok<3){
      fprintf(stderr, "ERROR: at least 3 columns required (X, Y and Z coordinates), with header line: your table seems to have only one or two."
                "Check the delimiter, comma, tab, pipe, and semi-column characters are accepted as column separators!"
                "If you don't care about camera Z coordinate (e.g. if your cloud is normalized to ground) and want a fixed value, you can put '0' for the third column and fix the value using -zCam\n");
      // byebye(true, argc==1);
    }
    int nori = csvPitchIndex+csvYawIndex+csvRollIndex;
    if(nori!= -3 && (csvPitchIndex < 0 || csvYawIndex < 0 || csvRollIndex < 0) ){
      fprintf(stderr, "ERROR: orientation of camera requires three columns,"
                " pitch, yaw and roll - respective column index found %d %d %d - (-1 == missing) please check your CSV file !\n",
                csvPitchIndex,
                csvYawIndex,
                csvRollIndex );
      byebye(true, argc==1);
    }

    if( (csvXIndex < 0 || csvYIndex < 0 || csvZIndex < 0) ){
      fprintf(stderr, "ERROR: X, Y and Z coordinates have not been parsed, "
                "  - respective column index found %d %d %d - (-1 == missing) please check your CSV file !\nRemember the first line should have X,Y,Z without double quotes.",
                csvXIndex,
                csvYIndex,
                csvZIndex );
      byebye(true, argc==1);
    }



    if(verbose) {
      fprintf(stderr, "Reading first line %s with separator '%s' in '%s'  \n", line, fsep, file_name_location);
    }

    while (fgets(line, 1024, fpLocations))
    {
      if(line[0] == '#'){
        if(verbose){
          fprintf(stderr, "Skipping line %s...\n",
                  line);
        }
        continue;
      }
      char *token, *str, *tofree;
      line[strcspn(line, "\r\n")] = 0;
      tofree = str = strdup((char*)(&line));  // We own str's memory now.

      if(strlen(line)< 5){
          fprintf(stderr, "WARNING: Line with only %d characters, skipping %s...\n",
              (int)strlen(line),
              line);
      }

      int tok2=0;
      token = strtok(str, fsep);
      while (token) {
        (tmptokens[tok2])=strdup(token);
        tok2++;
        token = strtok(NULL, fsep);
      }

      if(tok2!=tok){
        fprintf(stderr, "ERROR: CSV file has header with %d columns but this line parsed %d columns ! Please check your CSV file\n",
                tok,  tok2 );
        byebye(true, argc==1);
      }


      plotPositions[plotN].x =  atof( tmptokens[csvXIndex] );
      plotPositions[plotN].y =  atof( tmptokens[csvYIndex] );
      plotPositions[plotN].z =  atof( tmptokens[csvZIndex] );
      if(plotPositions[plotN].x==.0){
        fprintf(stderr, "WARNING: read coordinate X with 0.0 value... make sure it is correct \n");
      }
      if(plotPositions[plotN].y==.0){
        fprintf(stderr, "WARNING: read coordinate Y with 0.0 value... make sure it is correct \n");
      }
      if(plotPositions[plotN].z==.0){
        fprintf(stderr, "WARNING: read coordinate Z with 0.0 value... make sure it is correct \n");
      }
      plotPositions[plotN].z = plotPositions[plotN].z + zCam;

      if(csvPitchIndex == -1 && ori!=NULL){
        plotPositions[plotN].ori = new double[3];
        plotPositions[plotN].ori[0]  =   ori[0];
        plotPositions[plotN].ori[1]  =   ori[1];
        plotPositions[plotN].ori[2]  =   ori[2];
      }

      if(csvPitchIndex > -1){
        plotPositions[plotN].ori = new double[3];

        plotPositions[plotN].ori[0]  =  atof( tmptokens[csvPitchIndex] );
        if(verbose && plotPositions[plotN].ori[0]==.0){
          fprintf(stderr, "WARNING: read pitch angle with 0.0 degrees value... make sure it is correct \n");
        }
        plotPositions[plotN].ori[0]  = deg2rad(plotPositions[plotN].ori[0] );

        plotPositions[plotN].ori[1]  =  atof( tmptokens[csvYawIndex] );
        if(verbose && plotPositions[plotN].ori[1]==.0){
          fprintf(stderr, "WARNING: read yaw/azimuth angle with 0.0 degrees value... make sure it is correct \n");
        }
        plotPositions[plotN].ori[1]  = deg2rad(plotPositions[plotN].ori[1] );

        plotPositions[plotN].ori[2]  =  atof( tmptokens[csvRollIndex] );
        if(verbose && plotPositions[plotN].ori[2]==.0){
          fprintf(stderr, "WARNING: read roll/tilt angle with 0.0 degrees value... make sure it is correct \n");
        }
        plotPositions[plotN].ori[2]  = deg2rad(plotPositions[plotN].ori[2] );

      }

      if( csvOrastIndex  > -1 ){
        plotPositions[plotN].orast  =  atoi( tmptokens[csvOrastIndex] );
        if(verbose && plotPositions[plotN].orast==0){
          fprintf(stderr, "ERROR: parsed output raster size to 0 pixels check CSV file \n");
          byebye(true, argc==1);
        }
      } else {
        plotPositions[plotN].orast = orast;
      }

      if( csvProjIndex  > -1 ){
        int projj=-1;
        if(strcmp(tmptokens[csvProjIndex],"str")==0){
          projj=1;
        } else if(strcmp(tmptokens[csvProjIndex], "eqa")==0){
          projj=2;
        } else if(strcmp(tmptokens[csvProjIndex],"eqd")==0){
          projj=3;
        } else if(strcmp(tmptokens[csvProjIndex],"ort")==0){
          projj=4;
        } else if(strcmp(tmptokens[csvProjIndex],"rct")==0){
          projj=5;
        }
        if(projj < 0){
          fprintf(stderr, "ERROR: cannot parse lens projection '%s' in 'proj' column CSV file. Please make sure it is either 'eqd', 'eqa', 'str', 'ort' or 'rct' - see help with -h flag \n",
                  tmptokens[csvProjIndex] );
          byebye(true, argc==1);
        }

        plotPositions[plotN].proj = projj;
      } else {
        plotPositions[plotN].proj = proj;
      }
      free(tofree);


      plotN++;

    }
    if(verbose)  fprintf(stderr, "Read %d plot positions...\n", plotN);
  }

  if(verbose) fprintf(stderr, "Finished reading '%s'  \n", file_name_location);

  fclose(fpLocations);

  if (!lasreadopener.active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    usage(argc == 1);
    byebye(true, argc==1);
  }



  quantizer *collector = new quantizer( plotN, plotPositions,
                                       zenCut,   toLog,  toDb, weight,
                                       file_name_location );

  // if(verbose) fprintf(stderr,"Reading %d LAS/LAZ files sampled on %d plots\n", nPositions);
  polarCoordinate polCrt;
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

      if ( progress && ((lasreader->p_count % progress) == 0))
      {
        if(q==1000) fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b%03d / 1000",  (int) ((float)lasreader->p_count/(float)lasreader->npoints*1000.0)) ;
        else  fprintf(stderr, "\b\b\b%02d%%",  (int) ((float)lasreader->p_count/(float)lasreader->npoints*q)) ;
      }

      // if(  lasreader->point.get_z() < zCam   ){
      //   continue;
      // }

      for(int i=0; i<plotN; ++i){
        // grab coordinates

        lasreader->point.compute_coordinates();
        // convert to plot center reference, if distance above maxdist parameter, then continue....

        original2cameracoords(&lasreader->point, plotPositions[i]);

        polCrt = crtPlot2polar(&lasreader->point);
        if( polCrt.distance > maxdist ) continue;

        // fprintf(stderr, "\n 1111.aaaa%f\n " , polCrt.planar.x );
        camera2image( &polCrt, plotPositions[i].proj, 1.0);

        // point is below cutoff angle
        if( polCrt.zenith > zenCut ) continue;

        // if(polCrt.planar.isImage)
        // fprintf(stderr, "\n 2222.aaaa%f\n " , polCrt.planar.x );
        collector->image2grid(i,  &polCrt  );
        // fprintf(stderr, "\n 2222.aaaa%f\n " , polCrt.planar.x );

      }


    }

    #ifdef _WIN32
      if (verbose) fprintf(stderr,"\ntotal time: %g sec   for %I64d points\n", taketime()-start_time,   lasreader->p_count);
    #else
      if (verbose) fprintf(stderr,"\ntotal time: %g sec   for %lld points\n", taketime()-start_time, lasreader->p_count);
    #endif

    lasreader->close();
    delete lasreader;
  }

  fprintf(stderr, "\b\b\b100%%\n\nFINALIZING.....\n" );
  float *gapFractions;

  gapFractions = collector->finalizePlotDomes(verbose);
  char bbb[12048];
  sprintf(bbb, "%s_zenCut%.2f.out", file_name_location,   (rad2deg(zenCut))  );
  fpLocations = fopen(file_name_location, "r");
  FILE *fpLocationsout = fopen(bbb, "w");
  char line[1024];
  char lineout[2048];
  fgets(line, 1024, fpLocations);
  line[strcspn(line, "\r\n")] = 0;
  strcat(strcat(line, fsep), "GapFraction\n");
  fputs(line,  fpLocationsout);
  int c=0;
  while (fgets(line, 1024, fpLocations))
  {
    if(line[0] == '#'){
      if(verbose){
        fprintf(stderr, "Skipping line %s...\n",
                line);
      }
      continue;
    }
    line[strcspn(line, "\r\n")] = 0;
    sprintf(lineout, "%s%s%f\n", line, fsep, gapFractions[c]);
    fputs(lineout,  fpLocationsout);
    c++;
  }
  fclose(fpLocations);
  fclose(fpLocationsout);
  return 0;
}
