// Just flying over VXL map
// by Tom Dobrowolski 2003-04-08

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <list>
#include "sysmain.h"
#include "voxlap5.h"

#include <fstream>
using namespace std;

extern void get_canvas_camera(dpoint3d &ipos, dpoint3d &istr, dpoint3d &ihei, dpoint3d &ifor);

//Player position variables:
#define NOCLIPRAD 7
#define CLIPRAD 5
#define CLIPVEL 8.0
#define MINVEL 0.5
dpoint3d ipos, istr, ihei, ifor, ivel, ivelang;


struct draw_point
{
	float x, y, z;
};

std::list<draw_point> points_to_draw;

long initapp (long argc, char **argv)
{
	char *level = NULL;
	char *stlfile = NULL;
	
	xres = 1024; yres = 768; colbits = 32; fullscreen = 0;

	initvoxlap();

	vx5.maxscandist = 2048;
	vx5.fogcol = 0x808080;
	vx5.lightmode = 1;
	vx5.curcol = 0x808080;
	setsideshades(0,4,1,3,2,2);
// load a default scene
	loadnul (&ipos,&istr,&ihei,&ifor);

	// remove all voxels
	{
		lpoint3d pmin, pmax;
		pmin.x = 0; pmin.y = 0; pmin.z = 0;
		pmax.x = 1023; pmax.y = 1023; pmax.z = 1023;
		setrect(&pmin, &pmax, -1);
	}

	if(stlfile){
		ifstream ifs(stlfile, ios::binary);
		if(!ifs){
			char mess[1024];
			sprintf(mess, "couldn't load stl file - %s", stlfile);
			MessageBox(NULL, mess, "Error!", MB_OK);
			return -1;
		}

		int type = 0;

		while(1){
			ifs.read((char *)(&type), sizeof(int));
			if(type == 1){// triangle
				double x[9];
				ifs.read((char *)(x), 9*sizeof(double));
				point3d p1, p2, p3;
				p1.x = (float)x[0];
				p1.y = (float)x[1];
				p1.z = 256.0f - (float)x[2];
				p2.x = (float)x[3];
				p2.y = (float)x[4];
				p2.z = 256.0f - (float)x[5];
				p3.x = (float)x[6];
				p3.y = (float)x[7];
				p3.z = 256.0f - (float)x[8];
				settri(&p1, &p2, &p3, 0);
			}
			else if(type == 2){// flood fill point
				double p[3];
				double e[6];
				ifs.read((char *)(p), 3*sizeof(double));
				ifs.read((char *)(e), 6*sizeof(double));
				setfloodfill3d ((long)p[0], (long)p[1], 256 - (long)p[2], (long)e[0], (long)e[1], 256 - (long)e[5], (long)e[3], (long)e[4], 256 - (long)e[2]);
				draw_point dp;
				dp.x = (float)p[0]; dp.y = (float)p[1]; dp.z = 256.0f - (float)p[2];
				setcube((long)dp.x, (long)dp.y, (long)dp.z, 982374);
			}
			else{
				break;
			}
		}
	}

	updatevxl();
	
	vx5.fallcheck = 1;

	ivel.x = ivel.y = ivel.z = 0;
	ivelang.x = ivelang.y = ivelang.z = 0;
	
	return(0);
}

void uninitapp ()
{
	uninitvoxlap();
}
	 
static float fov = 0.6f;

void doframe ()
{
	long frameptr, pitch, xdim, ydim;
	
	if (startdirectdraw(&frameptr,&pitch,&xdim,&ydim)) {
		voxsetframebuffer(frameptr,pitch,xdim,ydim);

		get_canvas_camera(ipos, istr, ihei, ifor);

		setcamera(&ipos,&istr,&ihei,&ifor,xres*.5f,yres*.5f,xres*fov); // .5f
		
		opticast();

		stopdirectdraw();
		nextpage();
	}
}

