#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define INF 999999.9f

#define WIDTH 3840
#define HEIGHT 2160

typedef struct vec3
{
	float x, y, z;
} vec3;

static inline void vec_add(vec3* v1, vec3* v2, vec3* res)
{
	res->x = v1->x + v2->x;
	res->y = v1->y + v2->y;
	res->z = v1->z + v2->z;
}

static inline void vec_sub(vec3* v1, vec3* v2, vec3* res)
{
	res->x = v1->x - v2->x;
	res->y = v1->y - v2->y;
	res->z = v1->z - v2->z;
}

static inline void vec_scale(vec3* v, float scale)
{
	v->x *= scale;
	v->y *= scale;
	v->z *= scale;
}

static inline void vec_scale_save(vec3* v, vec3* res, float scale)
{
	res->x = v->x * scale;
	res->y = v->y * scale;
	res->z = v->z * scale;
}

static inline float vec_length(vec3* v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static inline void normalize(vec3* v)
{
	float length = vec_length(v);

	v->x /= length;
	v->y /= length;
	v->z /= length;
}

static inline float dot(vec3* v1, vec3* v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

typedef struct 
{
   unsigned short int type;                 /* Magic identifier            */
   unsigned int size;                       /* File size in bytes          */
   unsigned short int reserved1, reserved2;
   unsigned int offset;                     /* Offset to image data, bytes */
} __attribute__((packed)) bmp_header;

typedef struct 
{
   unsigned int size;               /* Header size in bytes      */
   int width,height;                /* Width and height of image */
   unsigned short int planes;       /* Number of colour planes   */
   unsigned short int bits;         /* Bits per pixel            */
   unsigned int compression;        /* Compression type          */
   unsigned int imagesize;          /* Image size in bytes       */
   int xresolution,yresolution;     /* Pixels per meter          */
   unsigned int ncolours;           /* Number of colours         */
   unsigned int importantcolours;   /* Important colours         */
} __attribute__((packed)) bmp_info_header;

typedef struct Color 
{
	unsigned char b, g, r;
} Color;

typedef struct Color_f 
{
	float b, g, r;
} Color_f;

typedef struct Sphere 
{
	vec3 origin;
	
	Color_f ambient;
	Color_f diffuse;
	Color_f specular;
	
	float shine;
	float reflection;
	float r;
} Sphere;

static inline void color_mul(Color_f* c1, Color_f* c2, Color_f* res)
{
	res->r = c1->r * c2->r;
	res->g = c1->g * c2->g;
	res->b = c1->b * c2->b;
}

static inline void color_scale(Color_f* c1, Color_f* res, float scale)
{
	res->r = c1->r * scale;
	res->g = c1->g * scale;
	res->b = c1->b * scale;
}

static inline void color_add(Color_f* c1, Color_f* c2, Color_f* res)
{
	res->r = c1->r + c2->r;
	res->g = c1->g + c2->g;
	res->b = c1->b + c2->b;
}

void save_bmp(void* buffer)
{
	FILE* fp = fopen("image.bmp", "wb");
	bmp_header header;
	bmp_info_header info;

	//header
	header.type 	 	  = 0x4D42; // 'BM'
	header.reserved1 	  = 0;
    header.reserved2 	  = 0;
	header.size 	 	  = 54 + WIDTH * HEIGHT * 3;
	header.offset 	      = 54;

	//info header
	info.size 		 	  = 40;
	info.width 		 	  = WIDTH;
	info.height 	 	  = HEIGHT;
	info.compression 	  = 0;
	info.planes 	 	  = 1;
	info.bits 		      = 24;
	info.imagesize	   	  = 0;
	info.xresolution	  = 0;
	info.yresolution 	  = 0;
	info.ncolours 	 	  = 0;
	info.importantcolours = 0;

	fwrite(&header, sizeof(header), 1, fp);
	fwrite(&info, 	sizeof(info), 1, fp);
	fwrite(buffer, 	WIDTH * HEIGHT * 3, 1, fp);

	free(buffer);

	fclose(fp);
}

float intersect_distance(Sphere* object, vec3* camera, vec3* diff)
{
	vec3 o_min_c;

	vec_sub(camera, &object->origin, &o_min_c);

	float o_min_c_len = vec_length(&o_min_c);
	float c = (o_min_c_len * o_min_c_len - object->r * object->r);
	float b = 2 * dot(&o_min_c, diff);
	float delta = b*b - 4*c;

	if (delta > 0)
	{
		float t1 = (-b + sqrtf(delta)) / 2;
		float t2 = (-b - sqrtf(delta)) / 2;

		if (t1 > 0 && t2 > 0)
			return t1 < t2 ? t1 : t2;
	}

	return INF;
}

float DIST;

Sphere* nearest_object(Sphere* objects, vec3* origin, vec3* diff, int cnt)
{
	float min_distance = INF, dist;
	int index = -1;

	for (int i = 0; i < cnt; ++i)
	{
		dist = intersect_distance(&objects[i], origin, diff);

		if (dist < min_distance)
		{
			min_distance = DIST = dist;
			index = i;
		}
	}

	if (index == -1)
	{
		DIST = INF;
		return NULL;
	}

	return &objects[index];
}

int main(void)
{
	Color (*screen)[WIDTH] = malloc(WIDTH * HEIGHT * 3);
	Sphere spheres[] = {
		{
			{-0.2, 0, -1}, //position
			{0, 0, 0.1},   //color (amb)
			{0, 0, 0.7},   //color (diff)
			{1, 1, 1},     //color (spec)
			100.0f,		   //shine
			0.5f, 		   //reflection
			0.7f		   //radius
		},
		{
			{0.1, -0.3, 0},  //position
			{0.1, 0, 0.1},       //color (amb)
			{0.7, 0, 0.7},    //color (diff)
			{1, 1, 1},    //color (spec)
			100.0f,		  //shine
			0.5f, 		  //reflection
			0.1f		  //radius
		},
		{
			{-0.3, 0, 0},  //position
			{0, 0.1, 0},   //color (amb)
			{0, 0.6, 0},   //color (diff)
			{1, 1, 1},     //color (spec)
			100.0f,		   //shine
			0.5f, 		   //reflection
			0.15f		   //radius
		},
		{
			{0, -9000, 0},  //position
			{0.1, 0.1, 0.1},   //color (amb)
			{0.6, 0.6, 0.6},   //color (diff)
			{1, 1, 1},     //color (spec)
			100.0f,		   //shine
			0.5f, 		   //reflection
			9000.0f - 0.7f   //radius
		},
	};

	Sphere light = {
		{5, 5, 5},
		{1, 1, 1},   //color (amb)
		{1, 1, 1},   //color (diff)
		{1, 1, 1},   //color (spec),
		0.0f,
		0.0f,
		0.0f
	};

	vec3 camera = {0, 0, 1};
	vec3 delta;

	float aspect_ratio = (float)HEIGHT / WIDTH;
	float pixel_width = 2.0f / WIDTH;

	if (screen == NULL)
	{
		printf("memalloc() failed!\n");
		return 1;
	}

	//clear the screen
	memset(screen, 0, WIDTH * HEIGHT * 3);

	clock_t start, end;
	double cpu_time_used;

	start = clock();

	for (int i = 0; i < HEIGHT; ++i)
	{
		for (int j = 0; j < WIDTH; ++j)
		{
			vec3 pixel = {-1 + j * pixel_width, -aspect_ratio + i * pixel_width, 0};
			vec3 light_delta, intersection, intersection_to_light, normal, shifted_point;
			Color_f illumination, diffused, specular;

			vec_sub(&pixel, &camera, &delta);
			normalize(&delta);

			Sphere* nearest = nearest_object(spheres, &camera, &delta, 4);

			if (nearest)
			{
				//intersection with sphere - intersection
				vec_scale(&delta, DIST);
				vec_add(&camera, &delta, &intersection);

				//normal to surface
				vec_sub(&intersection, &nearest->origin, &normal);
				normalize(&normal);

				vec3 temp;
				vec_scale_save(&normal, &temp, 1e-5);
				vec_add(&intersection, &temp, &shifted_point);

			
				//intersection to light - light_delta
				vec_sub(&light.origin, &shifted_point, &light_delta);
				normalize(&light_delta);

				vec_sub(&light.origin, &intersection, &intersection_to_light);

				nearest_object(spheres, &shifted_point, &light_delta, 4);

				if (DIST < vec_length(&intersection_to_light))
				{
					continue;
				}

				//ambient light
				color_mul(&light.ambient, &nearest->ambient, &illumination);
				
				//diffuse light
				color_mul(&light.diffuse, &nearest->diffuse, &diffused);
				color_scale(&diffused, &diffused, dot(&light_delta, &normal));
				color_add(&illumination, &diffused, &illumination);

				// specular light
				vec3 intersection_to_camera, H;
				vec_sub(&camera, &intersection, &intersection_to_camera);
				normalize(&intersection_to_camera);

				vec_add(&light_delta, &intersection_to_camera, &H);
				normalize(&H);

				color_mul(&nearest->specular, &light.specular, &specular);
				color_scale(&specular, &specular, pow(dot(&normal, &H), nearest->shine / 4));

				illumination.r += specular.r;
				illumination.g += specular.g;
				illumination.b += specular.b;

				//clip color
				if (illumination.r > 1)
				{
					illumination.r = 1.0f;
				}

				if (illumination.g > 1)
				{
					illumination.g = 1.0f;
				}

				if (illumination.b > 1)
				{
					illumination.b = 1.0f;
				}

				// set pixel color
				screen[i][j].r = illumination.r * 255;
				screen[i][j].g = illumination.g * 255;
				screen[i][j].b = illumination.b * 255;
			}
		}
	}

	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	printf("Time: %f s\n", cpu_time_used);

	save_bmp(screen);

	return 0;
}