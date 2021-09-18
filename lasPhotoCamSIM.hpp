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
  double distance2d=0;
  point planar;
};



void camera2imageStr(polarCoordinate *pt, double c=1.0 ) {  
  double x, y;
  
  if(pt->distance2d==0) {x=.0; y=.0;} 
  else { 
    x = (tan(pt->zenith/2.0) * 2.0 * cos(pt->azimuth))/ tan(M_PI/4.0); 
    y = (tan(pt->zenith/2.0) * 2.0 * sin(pt->azimuth))/ tan(M_PI/4.0); 
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
  else if(pt->zenith > fov ) { 
    pt->planar.isImage=false;  
    } 
  else  { 
    x = tan(pt->zenith)  * cos(pt->azimuth); 
    y = tan(pt->zenith)  * sin(pt->azimuth); 
    pt->planar.x = x;  
    pt->planar.y = y;  
    pt->planar.isImage=true; 
  }
  
}

// NB requires that point coordinates are relative to camera space (original2cameracoords) and that polar coordinates are provided
void camera2image(polarCoordinate *pt, int projection, double c=1.0) {  
  
  if(projection==1) camera2imageStr(pt,c);
  else if(projection==2) camera2imageEqa(pt,c);
  else if(projection==3) camera2imageEqd(pt,c);
  else if(projection==4) camera2imageOrt(pt,c);
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
    int nAzimuths;
    int nZeniths;
    int nPlots;
    float ***grids;  
    float maxvalue=.0;
    int mult=1; 
    
    double maxx = .0; 
    double maxy = .0;
    
    float zenCut=90.0;
    int orast=180;
    bool toLog=false;
    bool toDb=false;
    float weight=1.0;
    point *plotCenters;
    float *plotGapFraction; 
    char *basename;
    
    quantizer(int az, int ze, int plots, point *plotPositions,  
              float zenCut1=90.0, 
              int orast1=180, bool toLog1=false,  bool toDb1=false, 
              float weight1=1.0,  char *basename1=NULL, int mult=1) { 
      
      this->mult = mult;
      this->plotGapFraction = new float[plots];
      this->nAzimuths=az;
      this->nZeniths=ze;
      
      if(basename1==NULL){
        this->basename=strdup("lasPhotoCamSIMoutputCSV");
      } else {
        this->basename=strdup(basename1); 
        }
      
      this->orast=orast1;
      this->toLog=toLog1;
      this->toDb=toDb1;
      this->weight=weight1;
      
      this->nPlots=plots;
 
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
                "zenCut=%.2f" 
                "\n", plotCenters[0].x, plotCenters[0].y,
                this->nZeniths,
                this->nAzimuths, 
                this->zenCut);
      
      this->grids = (float***)malloc(sizeof( *grids)  * plots);

      if (this->grids  )
      {
        int i;
        for (i = 0; i < plots; i++)
        {
          this->plotGapFraction[i]=0.0;
          this->grids[i] = (float**)malloc(sizeof(*this->grids[i])  * orast );   
           
          
          if (this->grids[i] )
          { 
            for (int j = 0; j < ZENITHS; j++)
            {
              this->grids[i][j] =  new float[orast];  
              memset( this->grids[i][j], 0, (orast)*sizeof(float) );
              for(int iii=0; iii < orast; iii++){
                this->grids[i][j][iii]=0.0f;
              }
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
 
 
   void image2grid(int plotn, polarCoordinate *pt, int projection=2,  bool verbose=false) {
     
     if(!pt->planar.isImage) return;
     int x, y;
     x = (int)floor(((double)this->orast * pt->planar.x + (double)this->orast)/2.0);
     y = (int)floor(((double)this->orast * pt->planar.y + (double)this->orast)/2.0);
     // if(x > (this->orast-1)) x--;
     // if(y==this->orast) y--;
     
     if(x < 0 || x >= this->orast || y < 0 || y >= this->orast )
       fprintf(stderr, "\n az=%f zen=%f ------- %f  \t  \n%.2f  %f \n%.2f  %f -- \n%d\t%d \t%f \n"  ,
               pt->azimuth, pt->zenith,
             (double)this->orast,
             this->orast * pt->planar.x,
             this->orast * pt->planar.y,
             pt->planar.x,
             pt->planar.y,
             x, y, ((double)this->orast * pt->planar.y + (double)this->orast) );
     // if(x < this->maxx)    this->maxx = x;
     // if(y < this->maxy)    this->maxy = y;
       
     this->grids[plotn][y][x] = this->grids[plotn][y][x] + (1.0/pow((pt->distance+0.05),weight));
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
     
     
   FILE *out= fopen(outfilename, "w");
   
   fprintf(out, "ncols         %d \n"
             "nrows         %d \n"
             "xllcorner     %f\n"
             "yllcorner     %f\n"
             "cellsize      %f \n"
             "NODATA_value  -1\n", this->orast , this->orast, 
             (plotCenters[plotn].x - (float)this->orast/( (float)this->orast/90.0) ), 
             (plotCenters[plotn].y - (float)this->orast/( (float)this->orast/90.0) ),
             (180.0/(float)this->orast) );
   
   
   

   int hits=0;
   for (int row = 0; row < this->orast; row++)
   {   
     for (int col = 0; col < this->orast; col++){
       
       float v=((float)(image[row][col])) * this->orast; 
        
       // fprintf(stderr, "\n ------- %.2f   "  , v);
       
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
      fprintf(out, "%.2f " , v) ; 
     }
     fprintf(out, "\n"   ) ; 
    }
 
   if(ferror (out)) fprintf(stderr, "ERROR: File %s write error \n", outfilename); 
   
   fclose(out);  
   if(verbose) fprintf(stderr, "Written file %s with  image for plot %d\n", outfilename, plotn);  
    
   
   this->plotGapFraction[plotn] = 100.0 - ((double)hits / ((double) AZIMUTHS*(ceil(zenCut*2))) * 100.0) ;
    
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
  fprintf(stderr,"lasPhotoCamSIM -i in.las -loc plotPositions.csv -verbose  -zCam 0.0 -zenithCut 89 -orast -log \n"); 
  fprintf(stderr,"lasPhotoCamSIM -h\n");
  fprintf(stderr,"-orast <size of square in pizels> default=180 \n\t- exports a square grid in ESRI GRID ASCII format. Pixels represent the point counts. Size of grid  \n");
  fprintf(stderr,"-maxdist <distance in meters> default=1000.0 \n\t- will ignore any points that are further than this value from the center of the camera. \n");
  fprintf(stderr,"-log - converts pixel values, which represent point counts, to natural log scale (-orast must be also present). \n");
  fprintf(stderr,"-loc <file path> \n\t- is the path to a CSV file with X Y and Z coordinates - with header - other columns can be present and will be saved in output. Comma, tab, pipe, space, column and semi-column characters are accepted as column separators.\n");
  fprintf(stderr,"-zCam <height value in meters> default=0.0m \n\t- height of camera - NB this is in absolute height with respect to the point cloud, so if your point cloud is normalized (e.g. a canopy height model) then 1.3m will be 1.3m from the ground.  \n");
  fprintf(stderr,"-zenCut <Zenith angle in degrees> default=89° \n\t- At 90° the points will be at the horizon, potentially counting million of \n\tpoints: a smaller Zenith angle will ignore points lower than that angle \n\t(e.g. at 1km distance any point lower than 17.5 m height with respect to the camera ) .  \n");
  fprintf(stderr,"Output: the CSV file with an extra added column with Gap Fraction in percentage and, if '-orast' parameter is present, raster images 180x180 pixels in ESRI GRID ASCII format (https://en.wikipedia.org/wiki/Esri_grid).  \n");
  fprintf(stderr,"Version 0.95: for feedback contact author: Francesco Pirotti, francesco.pirotti@unipd.it  \n");
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

void original2cameracoords(LASpoint *pt, double x, double y, double z) {  
  pt->coordinates[0] = pt->coordinates[0]-x;
  pt->coordinates[1] = pt->coordinates[1]-y;
  pt->coordinates[2] = pt->coordinates[2]-z;
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
