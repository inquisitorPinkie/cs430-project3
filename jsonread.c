
#define T_CAMERA 1
#define T_SPHERE 2
#define T_PLANE 3
#define T_LIGHT 4
#define T_CYLINDER 5

int line = 1;

int next_c(FILE* file)
{
	int c = fgetc(file);
	if (c == 10) {
		line ++;
	}
	if (c == EOF) {
		fprintf(stderr, "Error: unexpected EOF\n");
		exit(1);
	}
	return c;
}
void expect_c(FILE* file, int d)
{
	int c = next_c(file);
	if (c == d) return;
	fprintf(stderr, "Error: expected %c got %c on line %d\n", d, c, line);
	exit(1);
}
void skip_ws(FILE* file)
{
	int c = next_c(file);
	while(isspace(c))
	{
		c = next_c(file);
	}
	ungetc(c, file);
}

float next_number(FILE* file)
{
	float val;

	int res = fscanf(file, "%f", &val);
	// error
	if(res == 1)
		return val;
	fprintf(stderr, "Error: Could not read number on line %d\n", line);
	exit(1);
}

float* next_vector(FILE* file)
{
	float* v = malloc(3 * sizeof(float));
	expect_c(file, '[');
	skip_ws(file);
	v[0] = next_number(file);
	skip_ws(file);
	expect_c(file, ',');
	skip_ws(file);
	v[1] = next_number(file);
	skip_ws(file);
	expect_c(file, ',');
	skip_ws(file);
	v[2] = next_number(file);
	skip_ws(file);
	expect_c(file, ']');
	return v;
}

char* parse_string(FILE* file)
{
	expect_c(file, '"');
	
	int max_size = 128;
	
	char buffer[max_size];
	int buff_size = 0;
	
	int c = next_c(file);
	while(c != '"' && buff_size < max_size)
	{
		
		if (c < 32 || c > 126)
		{
			fprintf(stderr, "Error: only readable ascii characters are allowed. on line %d\n", line);
			exit(1);
		}
		
		buffer[buff_size] = c;
		buff_size ++;
		c = next_c(file);
	}
	
	buffer[buff_size] = 0;
	
	return strdup(buffer);
}

Scene read_scene(char* json_name)
{
	FILE * json = fopen(json_name, "r");
	Scene scene;

	int c;
	
	skip_ws(json);
	
	// find beginning of file
	expect_c(json, '[');
	
	skip_ws(json);
	
	c = next_c(json);
	if (c == ']')
	{
		fprintf(stderr, "Warning: empty scene file.\n");
		return scene;
	}

	ungetc(c, json);
	skip_ws(json);
	
	int set_camera_width = 0;
	float camera_width = 0;
	int set_camera_height = 0;
	float camera_height = 0;
	
	while(1)
	{
		expect_c(json, '{');
		
		skip_ws(json);

		// parse an object
		char* key = parse_string(json);
		if (strcmp(key, "type") != 0) {
			fprintf(stderr, "Error: expected \"type\" key on line %d\n", line);
			exit(1);
		}
		free(key);
		
		skip_ws(json);
		
		expect_c(json, ':');
		
		skip_ws(json);
		
		char* type_value = parse_string(json);

		int objtype = 0;
		
		int set_radius = 0;
		float radius = 0; // just in case a sphere is read in
		int set_color = 0;
		float color[3];
		int set_normal = 0;
		float normal[3];
		int set_position = 0;
		float position[3];
		
		int set_specular = 0;
		float specular[3]; // default specular color of white
		specular[0] = 1.0;
		specular[1] = 1.0;
		specular[2] = 1.0;

		float angular_a0 = 0;
		float theta = 0;
		float radial_a0 = 0;
		float radial_a1 = 0;
		float radial_a2 = 1;

		float shinyness = 20;

		float reflectivity = 0;

		float transparency = 0;
		float ior = 1;

		// for cylinders
		int set_height = 0;
		float height = 0;
		int set_basis1 = 0;
		int set_basis2 = 0;
		float basis1[3];
		float basis2[3];

		Object new_object = scene.objects[scene.num_objects];
		
		if(strcmp(type_value, "camera") == 0) {
			objtype = T_CAMERA;
		} else if(strcmp(type_value, "sphere") == 0) {
			objtype = T_SPHERE;
		} else if(strcmp(type_value, "plane") == 0) {
			objtype = T_PLANE;
		} else if(strcmp(type_value, "light") == 0) {
			objtype = T_LIGHT;
		} else if(strcmp(type_value, "cylinder") == 0) {
			objtype = T_CYLINDER;
		} else {
			
			fprintf(stderr, "Unknown type \"%s\" on line %d\n", type_value, line);
			exit(1);
		}
		
		free(type_value);
		
		// copy the information into the new object
		new_object.kind = objtype;

		int finish = 0;

		while(1) 
		{
			skip_ws(json);
			c = next_c(json);

			if (c == '}')
			{
				// stop parsing object
				break;
			}
			else
			{
				if(finish)
				{
					fprintf(stderr, "Expected , and got end of object on line %d\n", line);
					exit(1);
				}

				// read another field
				skip_ws(json);
				char* key = parse_string(json);
				skip_ws(json);
				expect_c(json, ':');
				skip_ws(json);
				
				//printf("Key: %s on line %d\n", key, line);

				if(strcmp(key, "width") == 0)
				{
					if(objtype == T_CAMERA)
					{
						float value = next_number(json);
						camera_width = value;
						set_camera_width = 1;
					}
				}
				else if(strcmp(key, "height") == 0)
				{
					if(objtype == T_CAMERA)
					{
						float value = next_number(json);
						camera_height = value;
						set_camera_height = 1;
					}
					if(objtype == T_CYLINDER)
					{
						height = next_number(json);
						set_height = 1;
					}
				}
				else if(strcmp(key, "reflectivity") == 0)
				{
					float value = next_number(json);
					reflectivity = value;
				}
				else if(strcmp(key, "angular-a0") == 0)
				{
					float value = next_number(json);
					angular_a0 = value;
				}
				else if(strcmp(key, "theta") == 0)
				{
					float value = next_number(json);
					theta = value;
				}
				else if(strcmp(key, "radial-a0") == 0)
				{
					float value = next_number(json);
					radial_a0 = value;
				}
				else if(strcmp(key, "radial-a1") == 0)
				{
					float value = next_number(json);
					radial_a1 = value;
				}
				else if(strcmp(key, "radial-a2") == 0)
				{
					float value = next_number(json);
					radial_a2 = value;
				}
				else if(strcmp(key, "refractivity") == 0)
				{
					float value = next_number(json);
					transparency = value;
				}
				else if(strcmp(key, "ior") == 0)
				{
					float value = next_number(json);
					ior = value;
				}
				else if(strcmp(key, "specular_color") == 0)
				{
					float* value = next_vector(json);
					specular[0] = value[0];
					specular[1] = value[1];
					specular[2] = value[2];
					set_specular = 1;
					//set_gloss = 1;
				}
				else if(strcmp(key, "radius") == 0)
				{
					float value = next_number(json);
					radius = value;
					set_radius = 1;
				}
				else if(strcmp(key, "color") == 0 || strcmp(key, "diffuse_color") == 0)
				{
					float* v3 = next_vector(json);
					color[0] = v3[0];
					color[1] = v3[1];
					color[2] = v3[2];
					set_color = 1;
					free(v3);
				}
				else if(strcmp(key, "position") == 0)
				{
					float* v3 = next_vector(json);
					position[0] = v3[0];
					position[1] = v3[1];
					position[2] = v3[2];
					set_position = 1;
					free(v3);
				}
				else if(strcmp(key, "basis1") == 0)
				{
					float* v3 = next_vector(json);
					basis1[0] = v3[0];
					basis1[1] = v3[1];
					basis1[2] = v3[2];
					set_basis1 = 1;
					free(v3);
				}
				else if(strcmp(key, "basis2") == 0)
				{
					float* v3 = next_vector(json);
					basis2[0] = v3[0];
					basis2[1] = v3[1];
					basis2[2] = v3[2];
					set_basis2 = 1;
					free(v3);
				}
				else if(strcmp(key, "normal") == 0 || strcmp(key, "direction") == 0)
				{
					float* v3 = next_vector(json);
					normal[0] = v3[0] + 0;
					normal[1] = v3[1] + 0;
					normal[2] = v3[2] + 0;
					set_normal = 1;
					free(v3);
				}
				else
				{
					fprintf(stderr, "Error: unknown property: %s on line %d\n", key, line);
					exit(1);
				}
				
				free(key);
				
				skip_ws(json);
				c = next_c(json);

				if(c != ',')
					finish = 1;

				ungetc(c, json);
			}
		}
		
		
		// error checking to make sure the correct attributes were read in
		// and set the attributes to the last object in the buffer

		if(objtype == T_CAMERA)
		{
			if(set_camera_height != 1)
			{
				fprintf(stderr, "Camera must have a height! Line %d\n", line);
				exit(1);
			}
			if(set_camera_width != 1)
			{
				fprintf(stderr, "Camera must have a width! Line %d\n", line);
				exit(1);
			}
			if(set_position == 1)
				fprintf(stderr, "Warning, Camera does not use position at this time.\n");
			if(set_normal == 1)
				fprintf(stderr, "Warning, Camera does not use a normal vector at this time.\n");
			
			scene.camera_width = camera_width;
			scene.camera_height = camera_height;
			
		}
		if(objtype == T_SPHERE)
		{
			if(set_radius != 1)
			{
				fprintf(stderr, "Sphere must have a defined radius! Line %d\n", line);
				exit(1);
			}
			if(radius < 0)
			{
				fprintf(stderr, "Sphere must have a non-negative radius! Line %d\n", line);
				exit(1);
			}
			if(set_color != 1)
			{
				fprintf(stderr, "Object must have a color! Line %d\n", line);
				exit(1);
			}
			if(set_position != 1)
			{
				fprintf(stderr, "Object must have a position! Line %d\n", line);
				exit(1);
			}
			
			// compute properties of a sphere

			new_object.color[0] = color[0];
			new_object.color[1] = color[1];
			new_object.color[2] = color[2];

			new_object.specular[0] = specular[0];
			new_object.specular[1] = specular[1];
			new_object.specular[2] = specular[2];
			
			new_object.position[0] = position[0];
			new_object.position[1] = position[1];
			new_object.position[2] = position[2];
			new_object.a = shinyness;
			new_object.b = ior;
			new_object.c = reflectivity;
			new_object.d = radius;

			new_object.e = transparency;
		}
		if(objtype == T_PLANE)
		{
			if(set_color != 1)
			{
				fprintf(stderr, "Object must have a color! Line %d\n", line);
				exit(1);
			}
			if(set_position != 1)
			{
				fprintf(stderr, "Object must have a position! Line %d\n", line);
				exit(1);
			}
			if(set_normal != 1)
			{
				fprintf(stderr, "Plane must have a normal vector! Line %d\n", line);
				exit(1);
			}
			
			// calculate the properties of the plane

			new_object.color[0] = color[0];
			new_object.color[1] = color[1];
			new_object.color[2] = color[2];

			new_object.specular[0] = specular[0];
			new_object.specular[1] = specular[1];
			new_object.specular[2] = specular[2];
			
			normalize(normal);
			new_object.direction[0] = normal[0];
			new_object.direction[1] = normal[1];
			new_object.direction[2] = normal[2];

			new_object.a = shinyness;
			new_object.b = ior;
			new_object.c = reflectivity;
			new_object.d = normal[0] * position[0] + normal[1] * position[1] + normal[2] * position[2];
			
			new_object.e = transparency;
		}
		if(objtype == T_CYLINDER)
		{
			if(set_color != 1)
			{
				fprintf(stderr, "Object must have a color! Line %d\n", line);
				exit(1);
			}
			if(set_position != 1)
			{
				fprintf(stderr, "Object must have a position! Line %d\n", line);
				exit(1);
			}
			if(set_basis1 != 1)
			{
				fprintf(stderr, "Object must have a basis vector 1! Line %d\n", line);
				exit(1);
			}
			if(set_basis2 != 1)
			{
				fprintf(stderr, "Object must have a basis vector 2! Line %d\n", line);
				exit(1);
			}
			if(set_height != 1)
			{
				fprintf(stderr, "Object must have a height! Line %d\n", line);
				exit(1);
			}
			if(set_radius != 1)
			{
				fprintf(stderr, "Object must have a radius! Line %d\n", line);
				exit(1);
			}


			new_object.color[0] = color[0];
			new_object.color[1] = color[1];
			new_object.color[2] = color[2];

			new_object.specular[0] = specular[0];
			new_object.specular[1] = specular[1];
			new_object.specular[2] = specular[2];
			
			new_object.position[0] = position[0];
			new_object.position[1] = position[1];
			new_object.position[2] = position[2];

			normalize(basis1);
			normalize(basis2);

			new_object.direction[0] = basis1[0];
			new_object.direction[1] = basis1[1];
			new_object.direction[2] = basis1[2];

			new_object.a = basis2[0];
			new_object.b = basis2[1];
			new_object.c = basis2[2];
			new_object.d = height / 2.0;
			new_object.e = radius;

		}
		if(objtype == T_LIGHT)
		{
			if(set_color != 1)
			{
				fprintf(stderr, "Object must have a color! Line %d\n", line);
				exit(1);
			}
			if(set_position != 1)
			{
				fprintf(stderr, "Object must have a position! Line %d\n", line);
				exit(1);
			}
			
			new_object.color[0] = color[0];
			new_object.color[1] = color[1];
			new_object.color[2] = color[2];
			
			new_object.position[0] = position[0];
			new_object.position[1] = position[1];
			new_object.position[2] = position[2];
			
			normalize(normal);
			new_object.direction[0] = normal[0];
			new_object.direction[1] = normal[1];
			new_object.direction[2] = normal[2];
			//normalize(normal);
			new_object.a = radial_a2;
			new_object.b = radial_a1;
			new_object.c = radial_a0;
			new_object.d = angular_a0;
			new_object.e = theta == 0 ? 0 : cos(deg2rad(theta));
		}
		
		// increment number to move to the next object

		if(objtype == T_SPHERE || objtype == T_PLANE || objtype == T_CYLINDER)
		{
			if(scene.num_objects == MAX_OBJECTS){
				fprintf(stderr, "Error: Too many objects!\n");
				exit(1);
			}
			scene.objects[scene.num_objects] = new_object;
			scene.num_objects ++;
		}
		
		if(objtype == T_LIGHT)
		{
			if(scene.num_objects == MAX_LIGHTS){
				fprintf(stderr, "Error: Too many lights!\n");
				exit(1);
			}
			scene.lights[scene.num_lights] = new_object;
			scene.num_lights ++;
		}
		
		// continue with reading
		
		skip_ws(json);
		c = next_c(json);
		
		if (c == ',')
		{
			// continue
			skip_ws(json);
		}
		else if (c == ']') 
		{
			fclose(json);
			return scene;
		}
		else 
		{
			fprintf(stderr, "Error: Expected ] or , on line %d\n", line);
			exit(1);
		}
		
		// end parsing object
		skip_ws(json);
	}
	
	fclose(json);

	return scene;
}