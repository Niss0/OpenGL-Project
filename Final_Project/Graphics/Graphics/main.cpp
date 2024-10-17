#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <vector>
using namespace std;

const int WINDOW_WIDTH = 512;
const int WINDOW_HEIGHT = 512;

const int TEXTURE_HEIGHT = 512; // Texture dimensions, must be power of 2
const int TEXTURE_WIDTH = 512;

const double PI = 3.14156;

const int GRID_SIZE = 100; // Grid size for terrain

unsigned char texture0[TEXTURE_HEIGHT][TEXTURE_WIDTH][3]; // Texture data
double rotation_angle = 0; // Rotation angle for camera
double displacement = 0; // Used for updating camera or other parameters

// Terrain height maps and temporary buffers
double terrain[GRID_SIZE][GRID_SIZE] = { 0 };
double waterHeight[GRID_SIZE][GRID_SIZE] = { 0 };
double tempBuffer[GRID_SIZE][GRID_SIZE] = { 0 };

// Structure for representing 2D points (used for terrain and city building)
typedef struct {
	int x;
	int z;
} Point2D;

// Structure for representing 3D points (used for camera and terrain points)
typedef struct
{
	double x, y, z;
} Point3D;

// Structure for RGB color representation
typedef struct {
	double red, green, blue;
} Color;

// Camera and movement variables
Point3D cameraPosition = { 2, 11, 45 };
double movement_speed = 0;
double rotation_speed = 0;
double view_angle = PI;
Point3D viewDirection = { sin(view_angle), -0.3, cos(view_angle) };

Point2D cityLocation = { -100, -100 }; // Stores city position

bool cityExpandRight = false;
bool cityExpandLeft = false;
bool cityExpandUp = false;
bool cityExpandDown = false;

bool stopErosion = false; // Flag to stop terrain erosion
bool isTerrainForming = true; // Flag to indicate terrain formation

void UpdateTerrainMethod2();
void UpdateTerrainMethod3();

void SmoothTerrain();

// Initialize water height slightly below the terrain height
void initializeWaterHeight() {
	for (int i = 0; i < GRID_SIZE; i++) {
		for (int j = 0; j < GRID_SIZE; j++) {
			waterHeight[i][j] = terrain[i][j] - 0.001;
		}
	}
}

// Set texture for roads or bricks
void setTexture(int textureType) {
	int i, j;
	int randomValue;

	switch (textureType)
	{
	case 0: // bricks texture
		for (int i = 0; i < WINDOW_HEIGHT; i++)
		{
			for (int j = 0; j < WINDOW_WIDTH; j++)
			{
				randomValue = rand() % 20;
				if (i < WINDOW_HEIGHT / 2) { // Upper part - white bricks
					texture0[i][j][0] = 255 - randomValue;
					texture0[i][j][1] = 255 - randomValue;
					texture0[i][j][2] = 255 - randomValue;
				}
				else // Lower part - gray bricks
				{
					texture0[i][j][0] = 180 + randomValue;
					texture0[i][j][1] = 180 + randomValue;
					texture0[i][j][2] = 180 + randomValue;
				}
			}
		}
		break;
	case 1: // road texture
		for (i = 0; i < TEXTURE_HEIGHT; i++)
			for (j = 0; j < TEXTURE_WIDTH; j++)
			{
				randomValue = rand() % 30;
				if (i > TEXTURE_HEIGHT - 15 || i < 15 ||
					i < TEXTURE_HEIGHT / 2 && i >= TEXTURE_HEIGHT / 2 - 15 && j < TEXTURE_WIDTH / 2)
				{
					texture0[i][j][0] = 255 - randomValue; // White lines on the road
					texture0[i][j][1] = 255 - randomValue;
					texture0[i][j][2] = 255 - randomValue;
				}
				else  // Road surface
				{
					texture0[i][j][0] = 140 + randomValue;
					texture0[i][j][1] = 140 + randomValue;
					texture0[i][j][2] = 140 + randomValue;
				}
			}
		break;
	}
}

// Initialize scene, including terrain and textures
void initializeScene()
{
	int i, j;
	glClearColor(0.5, 0.7, 0.9, 0); // Background color
	glEnable(GL_DEPTH_TEST);

	srand(time(0)); // Seed random number generator

	// Initial terrain formation using two different methods
	for (i = 0; i < 4000; i++)
		UpdateTerrainMethod2();
	for (i = 0; i < 500; i++)
		UpdateTerrainMethod3();
	SmoothTerrain(); // Smooth the terrain

	for (i = 0; i < 15; i++)
		UpdateTerrainMethod3();

	initializeWaterHeight();

	// Road texture
	setTexture(1); // Assign texture type 1 (road)
	glBindTexture(GL_TEXTURE_2D, 1); // Bind texture to ID 1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, texture0);

	// Crosswalk texture
	setTexture(0); // Assign texture type 0 (crosswalk)
	glBindTexture(GL_TEXTURE_2D, 2); // Bind texture to ID 2
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, texture0);
}

// Modify the terrain with a linear erosion model
void UpdateTerrainMethod2()
{
	int x1, z1, x2, z2;
	double delta = 0.05;
	double slope, intercept;
	int i, j;

	if (rand() % 2 == 0)
		delta = -delta;

	x1 = rand() % GRID_SIZE;
	z1 = rand() % GRID_SIZE;

	x2 = rand() % GRID_SIZE;
	z2 = rand() % GRID_SIZE;

	if (x1 != x2)
	{
		slope = (z2 - z1) / ((double)(x2 - x1));
		intercept = z1 - slope * x1;

		for (i = 0; i < GRID_SIZE; i++)
			for (j = 0; j < GRID_SIZE; j++)
			{
				if (i < slope * j + intercept) terrain[i][j] += delta;
				else terrain[i][j] -= delta;
			}
	}
}

// Checks if a position is underwater (for sea level)
bool isUnderSeaLevel(int x, int z) {
	return x >= 0 && x < GRID_SIZE && z >= 0 && z < GRID_SIZE && 0 > terrain[x][z] && 0 > waterHeight[x][z];
}

// Checks if a position is underwater (for river level)
bool isUnderRiverLevel(int x, int z) {
	return x >= 0 && x < GRID_SIZE && z >= 0 && z < GRID_SIZE && 0 < waterHeight[x][z] && terrain[x][z] < waterHeight[x][z];
}

// Checks if the point is above water (both sea and river)
bool isAboveWater(int x, int z) {
	return x >= 0 && x < GRID_SIZE && z >= 0 && z < GRID_SIZE && terrain[x][z] > 0 && terrain[x][z] > waterHeight[x][z];
}

// Flood fill algorithm using stack to avoid recursion overflow
void floodFill(int x, int z)
{
	bool visited[GRID_SIZE][GRID_SIZE] = { false };
	vector <Point2D> stack;

	Point2D current = { x, z };
	stack.push_back(current);

	while (!stack.empty())
	{
		current = stack.back();
		stack.pop_back();

		x = current.x;
		z = current.z;
		if (isAboveWater(x, z) && isAboveWater(x, z - 1) && isAboveWater(x, z + 1) && isAboveWater(x - 1, z) && isAboveWater(x - 1, z - 1) && isAboveWater(x - 1, z + 1) && isUnderRiverLevel(x + 2, z) && isUnderRiverLevel(x + 3, z) && ((isUnderRiverLevel(x + 2, z + 1) && isUnderRiverLevel(x + 2, z + 2) && isUnderRiverLevel(x + 2, z + 3) && isUnderSeaLevel(x + 2, z + 4)) || (isUnderRiverLevel(x + 2, z - 1) && isUnderRiverLevel(x + 2, z - 2) && isUnderRiverLevel(x + 2, z - 3) && isUnderSeaLevel(x + 2, z - 4)))) {
			cityLocation.x = x;
			cityLocation.z = z;
			cityExpandRight = true;
			return;
		}
		else if (isAboveWater(x, z) && isAboveWater(x, z - 1) && isAboveWater(x, z + 1) && isAboveWater(x + 1, z) && isAboveWater(x + 1, z - 1) && isAboveWater(x + 1, z + 1) && isUnderRiverLevel(x - 2, z) && isUnderRiverLevel(x - 3, z) && ((isUnderRiverLevel(x - 2, z + 1) && isUnderRiverLevel(x - 2, z + 2) && isUnderRiverLevel(x - 2, z + 3) && isUnderSeaLevel(x - 2, z + 4)) || (isUnderRiverLevel(x - 2, z - 1) && isUnderRiverLevel(x - 2, z - 2) && isUnderRiverLevel(x - 2, z - 3) && isUnderSeaLevel(x - 2, z - 4)))) {
			cityLocation.x = x;
			cityLocation.z = z;
			cityExpandLeft = true;
			return;
		}
		else if (isAboveWater(x, z) && isAboveWater(x - 1, z) && isAboveWater(x + 1, z) && isAboveWater(x, z - 1) && isAboveWater(x - 1, z - 1) && isAboveWater(x + 1, z - 1) && isUnderRiverLevel(x, z + 2) && isUnderRiverLevel(x, z + 3) && ((isUnderRiverLevel(x + 1, z + 2) && isUnderRiverLevel(x + 2, z + 2) && isUnderRiverLevel(x + 3, z + 2) && isUnderSeaLevel(x + 4, z + 2)) || (isUnderRiverLevel(x - 1, z + 2) && isUnderRiverLevel(x - 2, z + 2) && isUnderRiverLevel(x - 3, z + 2) && isUnderSeaLevel(x - 4, z + 2)))) {
			cityLocation.x = x;
			cityLocation.z = z;
			cityExpandUp = true;
			return;
		}
		else if (isAboveWater(x, z) && isAboveWater(x - 1, z) && isAboveWater(x + 1, z) && isAboveWater(x, z + 1) && isAboveWater(x - 1, z + 1) && isAboveWater(x + 1, z + 1) && isUnderRiverLevel(x, z - 2) && isUnderRiverLevel(x, z - 3) && ((isUnderRiverLevel(x + 1, z - 2) && isUnderRiverLevel(x + 2, z - 2) && isUnderRiverLevel(x + 3, z - 2) && isUnderSeaLevel(x + 4, z - 2)) || (isUnderRiverLevel(x - 1, z - 2) && isUnderRiverLevel(x - 2, z - 2) && isUnderRiverLevel(x - 3, z - 2) && isUnderSeaLevel(x - 4, z - 2)))) {
			cityLocation.x = x;
			cityLocation.z = z;
			cityExpandDown = true;
			return;
		}
		else {
			if (x + 1 < GRID_SIZE && !visited[x + 1][z])
			{
				current.x = x + 1;
				current.z = z;
				stack.push_back(current);
			}
			if (x - 1 >= 0 && !visited[x - 1][z])
			{
				current.x = x - 1;
				current.z = z;
				stack.push_back(current);
			}
			if (z + 1 < GRID_SIZE && !visited[x][z + 1])
			{
				current.x = x;
				current.z = z + 1;
				stack.push_back(current);
			}
			if (z - 1 >= 0 && !visited[x][z - 1])
			{
				current.x = x;
				current.z = z - 1;
				stack.push_back(current);
			}
		}
		visited[x][z] = true;
	}
}

// Hydraulic erosion simulation
void hydraulicErosion() {
	int x = rand() % GRID_SIZE;
	int z = rand() % GRID_SIZE;
	bool erosionContinues = false;
	do
	{
		erosionContinues = false;
		Point3D currentPoint = { x, terrain[x][z], z };

		// Check the neighboring points for lower height
		if (x < 99) {
			Point3D neighborPoint = { x + 1, terrain[x + 1][z], z };
			if (neighborPoint.y < currentPoint.y) {
				currentPoint.y = neighborPoint.y;
				currentPoint.x = neighborPoint.x;
				currentPoint.z = neighborPoint.z;
				erosionContinues = true;
			}
		}

		if (z < 99) {
			Point3D neighborPoint = { x, terrain[x][z + 1], z + 1 };
			if (neighborPoint.y < currentPoint.y) {
				currentPoint.y = neighborPoint.y;
				currentPoint.x = neighborPoint.x;
				currentPoint.z = neighborPoint.z;
				erosionContinues = true;
			}
		}

		if (x > 0) {
			Point3D neighborPoint = { x - 1, terrain[x - 1][z], z };
			if (neighborPoint.y < currentPoint.y) {
				currentPoint.y = neighborPoint.y;
				currentPoint.x = neighborPoint.x;
				currentPoint.z = neighborPoint.z;
				erosionContinues = true;
			}
		}

		if (z > 0) {
			Point3D neighborPoint = { x, terrain[x][z - 1], z - 1 };
			if (neighborPoint.y < currentPoint.y) {
				currentPoint.y = neighborPoint.y;
				currentPoint.x = neighborPoint.x;
				currentPoint.z = neighborPoint.z;
				erosionContinues = true;
			}
		}

		// Apply erosion to the current point
		terrain[x][z] -= 0.0001;

		// Move to the next point
		x = currentPoint.x;
		z = currentPoint.z;
	} while (erosionContinues);
}

// Random walk terrain modification
void UpdateTerrainMethod3()
{
	double delta = 0.02;
	int x, z, count;
	int numSteps = 800;

	x = rand() % GRID_SIZE;
	z = rand() % GRID_SIZE;

	if (rand() % 2 == 0)
		delta = -delta;

	for (count = 1; count <= numSteps; count++)
	{
		terrain[z][x] += delta;
		switch (rand() % 4)
		{
		case 0: // right
			x++;
			break;
		case 1: // up
			z++;
			break;
		case 2: // down
			z--;
			break;
		case 3: // left
			x--;
			break;
		}
		x += GRID_SIZE;
		x = x % GRID_SIZE;
		z += GRID_SIZE;
		z = z % GRID_SIZE;
	}


}

// Apply a smoothing filter to the terrain
void SmoothTerrain()
{
	int i, j;

	for (i = 1; i < GRID_SIZE - 1; i++)
		for (j = 1; j < GRID_SIZE - 1; j++)
		{
			tempBuffer[i][j] = (terrain[i + 1][j - 1] + 2 * terrain[i + 1][j] + terrain[i + 1][j + 1] +
				2 * terrain[i][j - 1] + 4 * terrain[i][j] + 2 * terrain[i][j + 1] +
				terrain[i - 1][j - 1] + 2 * terrain[i - 1][j] + terrain[i - 1][j + 1]) / 16.0;
		}

	for (i = 1; i < GRID_SIZE - 1; i++)
		for (j = 1; j < GRID_SIZE - 1; j++)
			terrain[i][j] = tempBuffer[i][j];

}

// Set color based on terrain height
void SetTerrainColor(double height)
{
	height = fabs(height) / 10.0;

	if (height < 0.03) // sand
		glColor3d(0.9, 0.8, 0.7);
	else if (height < 0.5)// grass
		glColor3d(0.2 + height / 3, 0.5 - height / 2, 0);
	else
		glColor3d(1.2 * height, 1.2 * height, 1.3 * height);
}

// Draw the terrain grid
void DrawTerrain()
{
	int i, j;

	glColor3d(0, 0, 0.3);

	for (i = 1; i < GRID_SIZE; i++)
		for (j = 1; j < GRID_SIZE; j++)
		{
			glBegin(GL_POLYGON);
			SetTerrainColor(terrain[i][j]);
			glVertex3d(j - GRID_SIZE / 2, terrain[i][j], i - GRID_SIZE / 2);
			SetTerrainColor(terrain[i - 1][j]);
			glVertex3d(j - GRID_SIZE / 2, terrain[i - 1][j], i - 1 - GRID_SIZE / 2);
			SetTerrainColor(terrain[i - 1][j - 1]);
			glVertex3d(j - 1 - GRID_SIZE / 2, terrain[i - 1][j - 1], i - 1 - GRID_SIZE / 2);
			SetTerrainColor(terrain[i][j - 1]);
			glVertex3d(j - 1 - GRID_SIZE / 2, terrain[i][j - 1], i - GRID_SIZE / 2);
			glEnd();

			// Draw the river water surface
			glBegin(GL_POLYGON);
			glColor3d(0, 0.25, 0.6);
			glVertex3d(j - GRID_SIZE / 2, waterHeight[i][j], i - GRID_SIZE / 2);
			glVertex3d(j - GRID_SIZE / 2, waterHeight[i - 1][j], i - 1 - GRID_SIZE / 2);
			glVertex3d(j - 1 - GRID_SIZE / 2, waterHeight[i - 1][j - 1], i - 1 - GRID_SIZE / 2);
			glVertex3d(j - 1 - GRID_SIZE / 2, waterHeight[i][j - 1], i - GRID_SIZE / 2);
			glEnd();
		}


	// Draw water surface (transparent)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.3, 0.6, 0.8);
	glBegin(GL_POLYGON);
	glVertex3d(-GRID_SIZE / 2, 0, -GRID_SIZE / 2);
	glVertex3d(-GRID_SIZE / 2, 0, GRID_SIZE / 2);
	glVertex3d(GRID_SIZE / 2, 0, GRID_SIZE / 2);
	glVertex3d(GRID_SIZE / 2, 0, -GRID_SIZE / 2);
	glEnd();

	glDisable(GL_BLEND);
}

// Function to draw a cylindrical roof for buildings
void drawRoof(int sides, double topRadius, double bottomRadius, double topY, double downY)
{
	double alpha, teta = 2 * PI / sides;
	Color roofColor = { 0.620, 0.059, 0.204 };
	for (alpha = 0; alpha < 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);
		glColor3d(roofColor.red, roofColor.green, roofColor.blue);;
		glVertex3d(topRadius * sin(alpha), topY, topRadius * cos(alpha));// 1
		glVertex3d(topRadius * sin(alpha + teta), topY, topRadius * cos(alpha + teta)); //2
		glVertex3d(bottomRadius * sin(alpha + teta), downY, bottomRadius * cos(alpha + teta));// 3
		glVertex3d(bottomRadius * sin(alpha), downY, bottomRadius * cos(alpha)); //4
		glEnd();

		roofColor.red -= 0.05;
		roofColor.green -= 0.05;
		roofColor.blue -= 0.05;
	}
}

// Function to draw cylindrical walls for buildings
void drawCylinder(int sides, double topRadius, double bottomRadius)
{
	Color wallColor = { 0.996, 0.682, 0.416 };
	double alpha, teta = 2 * PI / sides;
	for (alpha = 0; alpha < 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);
		glColor3d(wallColor.red, wallColor.green, wallColor.blue);
		glVertex3d(sin(alpha), topRadius, cos(alpha));
		glVertex3d(sin(alpha + teta), topRadius, cos(alpha + teta));
		glVertex3d(sin(alpha + teta), bottomRadius, cos(alpha + teta));
		glVertex3d(sin(alpha), bottomRadius, cos(alpha));
		glEnd();
		wallColor.red -= 0.05;
		wallColor.green -= 0.05;
		wallColor.blue -= 0.05;
	}
}

// Function to draw windows on buildings
void drawWindows(int numWindows, int numFloor, Color* windowColor) {
	double radius = 1.01;
	int parts = (numWindows * 2) + 1;
	double alpha = 0, teta = PI / 2;
	double x2 = sin(alpha + teta);
	double y2 = 1;
	double z2 = cos(alpha + teta);
	double x1 = sin(alpha);
	double y1 = 1;
	double z1 = cos(alpha);
	double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
	double size = distance / parts;
	double dx = (x2 - x1) / parts;
	double dz = (z2 - z1) / parts;
	for (int i = 0; i < parts; i++) {
		if (i % 2 == 1) {
			glBegin(GL_POLYGON);
			glColor3d(windowColor->red, windowColor->green, windowColor->blue);
			glVertex3d(radius * (x1 + (i * dx)), numFloor - 0.33, radius * (z1 + (i * dz)));// left top
			glVertex3d(radius * (x1 + ((i + 1) * dx)), numFloor - 0.33, radius * (z1 + ((i + 1) * dz)));//right top
			glVertex3d(radius * (x1 + ((i + 1) * dx)), numFloor - 0.66, radius * (z1 + ((i + 1) * dz)));//right bottom
			glVertex3d(radius * (x1 + (i * dx)), numFloor - 0.66, radius * (z1 + (i * dz)));//left bottom
			glEnd();
		}
	}
}

// Function to draw individual floors of buildings
void drawBuildingFloor(int numFloor, int numWindows) {
	// Top layer
	drawCylinder(4, numFloor - 0.00, numFloor - 0.33);
	// Window layer
	drawCylinder(4, numFloor - 0.33, numFloor - 0.66);
	// Bottom layer 
	drawCylinder(4, numFloor - 0.66, numFloor - 1.00);

	// Draw windows on each side
	Color windowColor = { 0.0, 0.0, 0 };
	for (int i = 0; i <= 360; i += 90) {
		glPushMatrix();
		glRotated(i, 0, 1, 0);
		drawWindows(numWindows, numFloor, &windowColor);
		glPopMatrix();
		windowColor.blue -= 0.05;
	}
}

// Function to draw entire buildings with multiple floors
void drawBuilding(int numFloors, int numWindows) {
	// Draw each floor of the building
	for (int i = 1; i <= numFloors; i++) {
		drawBuildingFloor(i, numWindows);
	}

	drawRoof(4, 0, 1, numFloors + 1.00, numFloors + 0.00);
}

// Check if there's enough space to place a building
bool checkBuildingSpace(int x, int z) {
	return x - 1 >= 0 && x + 1 < GRID_SIZE && z - 1 >= 0 && z + 1 < GRID_SIZE && isAboveWater(x, z) && isAboveWater(x, z - 1) && isAboveWater(x, z + 1) && isAboveWater(x + 1, z) && isAboveWater(x + 1, z - 1) && isAboveWater(x + 1, z + 1) && isAboveWater(x - 1, z) && isAboveWater(x - 1, z - 1) && isAboveWater(x - 1, z + 1);
}

// Function to build roads to the right
void buildRoadRight(int x, int z)
{
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 2); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x - 1][z + 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x - 1][z - 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x][z - 1] + 0.1, x - GRID_SIZE / 2);
	glTexCoord2d(1, 2); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x][z + 1] + 0.1, x - GRID_SIZE / 2);
	glEnd();
}

// Function to build crosswalks to the right
void buildCrosswalkRight(int x, int z)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glTexCoord2d(0, 10.5); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x - 1][z + 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x - 1][z - 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x][z - 1] + 0.1, x - GRID_SIZE / 2);
	glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x][z + 1] + 0.1, x - GRID_SIZE / 2);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

// Build the city expanding to the right
void buildCityRight() {
	bool buildingPlaced = true;
	int x = cityLocation.x;
	int z = cityLocation.z;
	int counter = 0;
	while (x > 0 && isAboveWater(x, z) && isAboveWater(x, z - 1) && isAboveWater(x, z + 1) && isAboveWater(x - 1, z) && isAboveWater(x - 1, z - 1) && isAboveWater(x - 1, z + 1) && z - 2 >= 0 && z + 2 < GRID_SIZE) {
		// Place buildings
		if (counter % 2 == 0 && checkBuildingSpace(x, z - 2))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 1;
			int numOfFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - 2 - GRID_SIZE / 2, terrain[x][z - 2] + 0.15, x - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}
		if (counter % 2 == 0 && checkBuildingSpace(x, z + 2))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 2;
			int numOfFloors = (counter % 4) + 2;
			glPushMatrix();
			glTranslated(z + 2 - GRID_SIZE / 2, terrain[x][z + 2] + 0.15, x - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}

		// Draw roads or crosswalks
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		terrain[x][z - 2] = terrain[x][z + 2] = terrain[x][z - 1] = terrain[x][z + 1] = terrain[x][z];
		terrain[x - 1][z - 2] = terrain[x - 1][z + 2] = terrain[x - 1][z - 1] = terrain[x - 1][z + 1] = terrain[x - 1][z];
		waterHeight[x][z - 2] = waterHeight[x][z + 2] = waterHeight[x][z - 1] = waterHeight[x][z + 1] = waterHeight[x][z] = -1;
		waterHeight[x - 1][z - 2] = waterHeight[x - 1][z + 2] = waterHeight[x - 1][z - 1] = waterHeight[x - 1][z + 1] = waterHeight[x - 1][z] = -1;
		if (counter % 8 == 0) {//crosswalk
			buildCrosswalkRight(x, z);
		}
		else {//road
			buildRoadRight(x, z);
		}
		glDisable(GL_TEXTURE_2D);
		x--;
		counter++;
	}
}

// Build crosswalks to the left
void buildCrosswalkLeft(int x, int z)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glTexCoord2d(0, 10.5); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x + 1][z + 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x + 1][z - 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x][z - 1] + 0.1, x - GRID_SIZE / 2);
	glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x][z + 1] + 0.1, x - GRID_SIZE / 2);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

// Build roads to the left
void buildRoadLeft(int x, int z)
{
	glBegin(GL_POLYGON);
	glTexCoord2d(1, 2); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x + 1][z + 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x + 1][z - 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x][z - 1] + 0.1, x - GRID_SIZE / 2);
	glTexCoord2d(0, 2); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x][z + 1] + 0.1, x - GRID_SIZE / 2);
	glEnd();
}

// Build the city expanding to the left
void buildCityLeft() {
	bool buildingPlaced = true;
	int x = cityLocation.x;
	int z = cityLocation.z;
	int counter = 0;
	// Loop to place city roads and buildings
	while (x + 1 < GRID_SIZE && isAboveWater(x, z) && isAboveWater(x, z - 1) && isAboveWater(x, z + 1) && isAboveWater(x + 1, z) && isAboveWater(x + 1, z - 1) && isAboveWater(x + 1, z + 1) && z - 1 >= 0 && z + 1 < GRID_SIZE) {
		// Place buildings
		if (counter % 2 == 0 && checkBuildingSpace(x, z - 2))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 1;
			int numOfFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - 2 - GRID_SIZE / 2, terrain[x][z - 2] + 0.15, x - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}
		if (counter % 2 == 0 && checkBuildingSpace(x, z + 2))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 2;
			int numOfFloors = (counter % 4) + 2;
			glPushMatrix();
			glTranslated(z + 2 - GRID_SIZE / 2, terrain[x][z + 2] + 0.15, x - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		terrain[x][z - 2] = terrain[x][z + 2] = terrain[x][z - 1] = terrain[x][z + 1] = terrain[x][z];
		terrain[x + 1][z - 2] = terrain[x + 1][z + 2] = terrain[x + 1][z - 1] = terrain[x + 1][z + 1] = terrain[x + 1][z];
		waterHeight[x][z - 2] = waterHeight[x][z + 2] = waterHeight[x][z - 1] = waterHeight[x][z + 1] = waterHeight[x][z] = -1;
		waterHeight[x + 1][z - 2] = waterHeight[x + 1][z + 2] = waterHeight[x + 1][z - 1] = waterHeight[x + 1][z + 1] = waterHeight[x + 1][z] = -1;

		// Draw crosswalks or roads
		if (counter % 8 == 0) {//crosswalk
			buildCrosswalkLeft(x, z);
		}
		else {//road
			buildRoadLeft(x, z);
		}
		glDisable(GL_TEXTURE_2D);
		counter++;
		x++;
	}
}

// Build crosswalks upwards
void buildCrosswalkUp(int x, int z)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x - 1][z - 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 10.5); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x + 1][z - 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 10.5); glVertex3d(z - GRID_SIZE / 2, terrain[x + 1][z] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - GRID_SIZE / 2, terrain[x - 1][z] + 0.1, x - 1 - GRID_SIZE / 2);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

// Build roads upwards
void buildRoadUp(int x, int z)
{
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x - 1][z - 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 2); glVertex3d(z - 1 - GRID_SIZE / 2, terrain[x + 1][z - 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 2); glVertex3d(z - GRID_SIZE / 2, terrain[x + 1][z] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z - GRID_SIZE / 2, terrain[x - 1][z] + 0.1, x - 1 - GRID_SIZE / 2);
	glEnd();
}

// Build the city expanding upwards
void buildCityUp() {
	bool buildingPlaced = true;
	int x = cityLocation.x;
	int z = cityLocation.z;
	int counter = 0;
	// Loop to place city roads and buildings
	while (z > 0 && isAboveWater(x, z) && isAboveWater(x - 1, z) && isAboveWater(x + 1, z) && isAboveWater(x, z - 1) && isAboveWater(x - 1, z - 1) && isAboveWater(x + 1, z - 1) && x - 1 >= 0 && x + 1 < GRID_SIZE) {
		// Place buildings
		if (counter % 2 == 0 && checkBuildingSpace(x - 2, z))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 1;
			int numOfFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GRID_SIZE / 2, terrain[x - 2][z] + 0.15, x - 2 - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}
		if (counter % 2 == 0 && checkBuildingSpace(x + 2, z))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 2;
			int numOfFloors = (counter % 4) + 2;
			glPushMatrix();
			glTranslated(z - GRID_SIZE / 2, terrain[x + 2][z] + 0.15, x + 2 - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		terrain[x - 2][z] = terrain[x + 2][z] = terrain[x - 1][z] = terrain[x + 1][z] = terrain[x][z];
		terrain[x - 2][z - 1] = terrain[x + 2][z - 1] = terrain[x - 1][z - 1] = terrain[x + 1][z - 1] = terrain[x][z - 1];
		waterHeight[x - 2][z] = waterHeight[x + 2][z] = waterHeight[x - 1][z] = waterHeight[x + 1][z] = waterHeight[x][z] = -1;
		waterHeight[x - 2][z - 1] = waterHeight[x + 2][z - 1] = waterHeight[x - 1][z - 1] = waterHeight[x + 1][z - 1] = waterHeight[x][z - 1] = -1;
		if (counter % 8 == 0) {//crosswalk
			buildCrosswalkUp(x, z);
		}
		else {//road
			buildRoadUp(x, z);
		}
		glDisable(GL_TEXTURE_2D);
		counter++;
		z--;
	}
}

// Build crosswalks downwards
void buildCrosswalkDown(int x, int z)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0); glVertex3d(z - GRID_SIZE / 2, terrain[x - 1][z] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 10.5); glVertex3d(z - GRID_SIZE / 2, terrain[x + 1][z] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 10.5); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x + 1][z + 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x - 1][z + 1] + 0.1, x - GRID_SIZE / 2);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 1);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

// Build roads downwards
void buildRoadDown(int x, int z)
{
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0); glVertex3d(z - GRID_SIZE / 2, terrain[x - 1][z] + 0.1, x - 1 - GRID_SIZE / 2);
	glTexCoord2d(0, 2); glVertex3d(z - GRID_SIZE / 2, terrain[x + 1][z] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 2); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x + 1][z + 1] + 0.1, x + 1 - GRID_SIZE / 2);
	glTexCoord2d(1, 0); glVertex3d(z + 1 - GRID_SIZE / 2, terrain[x - 1][z + 1] + 0.1, x - 1 - GRID_SIZE / 2);
	glEnd();
}

// Build the city expanding downwards
void buildCityDown() {
	bool buildingPlaced = true;
	int x = cityLocation.x;
	int z = cityLocation.z;
	int counter = 0;
	// Loop to place city roads and buildings
	while (z + 1 < GRID_SIZE && isAboveWater(x, z) && isAboveWater(x - 1, z) && isAboveWater(x + 1, z) && isAboveWater(x, z + 1) && isAboveWater(x - 1, z + 1) && isAboveWater(x + 1, z + 1) && x - 1 >= 0 && x + 1 < GRID_SIZE) {
		// Place buildings
		if (counter % 2 == 0 && checkBuildingSpace(x - 2, z))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 1;
			int numOfFloors = (counter % 4) + 1;
			glPushMatrix();
			glTranslated(z - GRID_SIZE / 2, terrain[x - 2][z] + 0.15, x - 2 - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}
		if (counter % 2 == 0 && checkBuildingSpace(x + 2, z))
		{
			buildingPlaced = false;
			int numOfWindows = (counter % 4) + 2;
			int numOfFloors = (counter % 4) + 2;
			glPushMatrix();
			glTranslated(z - GRID_SIZE / 2, terrain[x + 2][z] + 0.15, x + 2 - GRID_SIZE / 2);
			glRotated(45, 0, 1, 0);
			glScaled(1, numOfFloors / 2 + 1, 1);
			drawBuilding(numOfFloors, numOfWindows);
			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		terrain[x - 2][z] = terrain[x + 2][z] = terrain[x - 1][z] = terrain[x + 1][z] = terrain[x][z];
		terrain[x - 2][z + 1] = terrain[x + 2][z + 1] = terrain[x - 1][z + 1] = terrain[x + 1][z + 1] = terrain[x][z + 1];
		waterHeight[x - 2][z] = waterHeight[x + 2][z] = waterHeight[x - 1][z] = waterHeight[x + 1][z] = waterHeight[x][z] = -1;
		waterHeight[x - 2][z + 1] = waterHeight[x + 2][z + 1] = waterHeight[x - 1][z + 1] = waterHeight[x + 1][z + 1] = waterHeight[x][z + 1] = -1;

		// Draw crosswalks or roads
		if (counter % 8 == 0) {//crosswalk
			buildCrosswalkDown(x, z);
		}
		else {//road
			buildRoadDown(x, z);
		}
		glDisable(GL_TEXTURE_2D);
		counter++;
		z++;
	}
}

// Determine where to build the city based on the city expansion direction
void buildCity() {
	if (cityExpandRight) {
		buildCityRight();
	}
	else if (cityExpandLeft) {
		buildCityLeft();
	}
	else if (cityExpandUp) {
		buildCityUp();
	}
	else if (cityExpandDown) {
		buildCityDown();
	}
}

// Display function that handles drawing all elements
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the frame buffer and Z-buffer

	glMatrixMode(GL_PROJECTION); // Set the matrix mode to projection
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 0.7, 300); // Define the camera perspective
	gluLookAt(cameraPosition.x, cameraPosition.y, cameraPosition.z, // Camera position
		cameraPosition.x + viewDirection.x, cameraPosition.y + viewDirection.y, cameraPosition.z + viewDirection.z,  // Point of interest
		0, 1, 0); // Up vector

	glMatrixMode(GL_MODELVIEW); // Set the matrix mode to model transformations
	glLoadIdentity(); // Reset the transformation matrix

	DrawTerrain(); // Draw the terrain

	// Apply hydraulic erosion if not stopped
	if (!stopErosion && cityLocation.x == -100) {
		for (int i = 0; i < 2; i++)
		{
			hydraulicErosion();
		}
	}
	else {
		if (cityLocation.x == -100) {
			int randomX = rand() % GRID_SIZE;
			int randomZ = rand() % GRID_SIZE;
			floodFill(randomX, randomZ); // Find the city location
		}
	}

	// Build the city if a location is found
	if (cityLocation.x != -100) {
		buildCity();
	}

	glutSwapBuffers(); // Display the frame buffer
}

// Idle function for continuous updating
void idle()
{
	displacement += 0.3;

	view_angle += rotation_speed; // Adjust view angle based on rotation speed

	viewDirection.x = sin(view_angle);
	viewDirection.z = cos(view_angle);

	// Move the camera based on speed and direction
	cameraPosition.x += movement_speed * viewDirection.x;
	cameraPosition.z += movement_speed * viewDirection.z;

	glutPostRedisplay(); // Redraw the scene
}

// Handle special key inputs for camera movement and control
void SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		rotation_speed += 0.001;
		break;
	case GLUT_KEY_RIGHT:
		rotation_speed -= 0.001;
		break;

	case GLUT_KEY_UP:
		movement_speed += 0.01;
		break;
	case GLUT_KEY_DOWN:
		movement_speed -= 0.01;
		break;

	case GLUT_KEY_PAGE_UP:
		cameraPosition.y += 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		cameraPosition.y -= 0.1;
		break;
	}
}

// Handle mouse input for toggling erosion
void mouse(int button, int state, int x, int y)
{ 
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		stopErosion = !stopErosion;
}

// Main function to initialize GLUT and start the program
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); // Set display mode
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("Terrain and City Builder");

	glutDisplayFunc(display); // Register display callback function
	glutIdleFunc(idle); // Register idle callback function

	glutSpecialFunc(SpecialKeys); // Register special keys callback function
	glutMouseFunc(mouse); // Register mouse callback function

	initializeScene(); // Initialize the scene

	glutMainLoop(); // Enter the GLUT event loop
}