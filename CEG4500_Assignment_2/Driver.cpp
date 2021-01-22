// Driver.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <GL/glut.h>
#include <fstream>

#include "ply.h"

class Slider {
private:
    bool active = false;
    float handleWidth;
    float min_x, max_x, min_y, max_y;
    float min_val, max_val, val;

public:
    Slider(float min_x, float max_x, float min_y, float max_y, float min_val, float max_val) {
        this->min_x = min_x;
        this->max_x = max_x;
        this->min_y = min_y;
        this->max_y = max_y;
        this->min_val = min_val;
        this->max_val = max_val;
        this->val = (min_val + max_val) / 2;
        this->handleWidth = max_y - min_y;
    }

    float getVal() {
        return this->val;
    }

    void draw() {
        glColor3f(.5, .5, .5);

        glBegin(GL_POLYGON);
        glVertex2f(min_x, max_y);
        glVertex2f(max_x, max_y);
        glVertex2f(max_x, min_y);
        glVertex2f(min_x, min_y);
        glEnd();

        if (active) {
            glColor3f(1, .1, .1);
        }
        else {
            glColor3f(.5, 0, 0);
            
        }
        

        float valx = (val - min_val) * (max_x - min_x - handleWidth) / (max_val - min_val) + min_x + handleWidth / 2;

        glBegin(GL_POLYGON);
        glVertex2f(valx - handleWidth / 2, max_y);
        glVertex2f(valx + handleWidth / 2, max_y);
        glVertex2f(valx + handleWidth / 2, min_y);
        glVertex2f(valx - handleWidth / 2, min_y);
        glEnd();
    }

    void updateVal(int mouse_x) {
        if (mouse_x < min_x + handleWidth / 2) {
            val = min_val;
        } else if (mouse_x > max_x - handleWidth / 2) {
            val = max_val;
        }
        else {
            val = (mouse_x - min_x - handleWidth / 2) * (max_val - min_val) / (max_x - min_x - handleWidth) + min_val;
        }
    }

    bool inBounds(int x, int y) {
        return x >= this->min_x && x <= this->max_x&& y >= this->min_y && y <= this->max_y;
    }

    void mouseClicked(int button, int state, int x, int y) {
        if (button == GLUT_LEFT_BUTTON) {
            if (active) {
                if (state == GLUT_UP) {
                    updateVal(x);
                    this->active = false;
                }
                else {
                    // shouldn't happen, but just to be safe?
                    if (this->inBounds(x, y)) {
                        updateVal(x);
                        this->active = true;
                    }
                }
            }
            else if (state == GLUT_DOWN) {
                if (this->inBounds(x, y)) {
                    updateVal(x);
                    this->active = true;
                }
            }
        }
    }

    void mouseDragged(int x, int y) {
        if (active) {
            updateVal(x);
        }
    }
};

int num_elems, num_verts, num_faces;
Vertex** vlist;
Face** flist;

// global for the longest line across object
float internal_diagonal;
// global for the corners of the bounding box
Vertex min_corner, max_corner;
// width and height of window, should automatically ajust other parts of program
const int WIDTH = 1500, HEIGHT = 900;
// ply file to read in
const char* FILE_NAME = "./icosahedron.ply";

float x_bound = 1;
float y_bound = 1;

int mouse_x = 0, mouse_y = 0;

Slider s1(WIDTH * 3.0 / 4.0 - WIDTH / 6.0, WIDTH * 3.0 / 4.0 + WIDTH / 6.0, HEIGHT / 2.0 - HEIGHT * 1.0 / 8.0 - HEIGHT / 16.0, HEIGHT / 2.0 - HEIGHT * 1.0 / 8.0, -180, 180);
Slider s2(WIDTH * 3.0 / 4.0 - WIDTH / 6.0, WIDTH * 3.0 / 4.0 + WIDTH / 6.0, HEIGHT / 2.0 - HEIGHT * 2.0 / 8.0 - HEIGHT / 16.0, HEIGHT / 2.0 - HEIGHT * 2.0 / 8.0, -180, 180);
Slider s3(WIDTH * 3.0 / 4.0 - WIDTH / 6.0, WIDTH * 3.0 / 4.0 + WIDTH / 6.0, HEIGHT / 2.0 - HEIGHT * 3.0 / 8.0 - HEIGHT / 16.0, HEIGHT / 2.0 - HEIGHT * 3.0 / 8.0, -180, 180);

GLuint plyIndex;

double lastTime = glutGet(GLUT_ELAPSED_TIME);
int nbFrames = 0;

// deleted everything that printed stuff, I wanted to speed up my process.
// I also added something to figure out the bounding box around the object as I'm reading it in, rather than searching through after its all read in.
void read_test(char* filename)
{
    int i, j, k;
    PlyFile* ply;
    int nelems;
    char** elist;
    int file_type;
    float version;
    int nprops;
    PlyProperty** plist;
    char* elem_name;
    int num_comments;
    char** comments;
    int num_obj_info;
    char** obj_info;

    /* open a PLY file for reading */
    ply = ply_open_for_reading(filename, &nelems, &elist, &file_type, &version);

    /* go through each kind of element that we learned is in the file */
    /* and read them */
    for (i = 0; i < nelems; i++) {
        /* get the description of the first element */
        elem_name = elist[i];
        plist = ply_get_element_description(ply, elem_name, &num_elems, &nprops);

        /* if we're on vertex elements, read them in */
        if (equal_strings((char*)"vertex", elem_name)) {

            /* create a vertex list to hold all the vertices */
            vlist = (Vertex**)malloc(sizeof(Vertex*) * num_elems);
            num_verts = num_elems;

            /* set up for getting vertex elements */

            ply_get_property(ply, elem_name, &vert_props[0]);
            ply_get_property(ply, elem_name, &vert_props[1]);
            ply_get_property(ply, elem_name, &vert_props[2]);

            /* grab all the vertex elements */
            for (j = 0; j < num_elems; j++) {

                /* grab and element from the file */
                vlist[j] = (Vertex*)malloc(sizeof(Vertex));
                ply_get_element(ply, (void*)vlist[j]);

                /* find bounding box for ply file */
                if (j == 0) {
                    memcpy(&min_corner, vlist[j], sizeof(Vertex));
                    memcpy(&max_corner, vlist[j], sizeof(Vertex));
                }
                else {
                    if (min_corner.x > vlist[j]->x) min_corner.x = vlist[j]->x;
                    if (min_corner.y > vlist[j]->y) min_corner.y = vlist[j]->y;
                    if (min_corner.z > vlist[j]->z) min_corner.z = vlist[j]->z;
                    if (max_corner.x < vlist[j]->x) max_corner.x = vlist[j]->x;
                    if (max_corner.y < vlist[j]->y) max_corner.y = vlist[j]->y;
                    if (max_corner.z < vlist[j]->z) max_corner.z = vlist[j]->z;
                }

            }
        }

        /* if we're on face elements, read them in */
        if (equal_strings((char*)"face", elem_name)) {

            /* create a list to hold all the face elements */
            flist = (Face**)malloc(sizeof(Face*) * num_elems);
            num_faces = num_elems;

            /* set up for getting face elements */

            ply_get_property(ply, elem_name, &face_props[0]);
            ply_get_property(ply, elem_name, &face_props[1]);

            /* grab all the face elements */
            for (j = 0; j < num_elems; j++) {

                /* grab and element from the file */
                flist[j] = (Face*)malloc(sizeof(Face));
                ply_get_element(ply, (void*)flist[j]);
            }
        }
    }

    /* grab and print out the comments in the file */
    comments = ply_get_comments(ply, &num_comments);

    /* grab and print out the object information */
    obj_info = ply_get_obj_info(ply, &num_obj_info);

    /* close the PLY file */
    ply_close(ply);
}

void calc_normals() {
    // ensure normal vector is set to zero
    for (int i = 0; i < num_verts; i++) {
        vlist[i]->nx = 0;
        vlist[i]->ny = 0;
        vlist[i]->nz = 0;
    }
    
    for (int i = 0; i < num_faces; i++) {
        // newells method for calculating surface vectors
        float nx = 0, ny = 0, nz = 0;
        for (int j = 0; j < flist[i]->nverts; j++) {
            Vertex* current = vlist[flist[i]->verts[j]];
            Vertex* next = vlist[flist[i]->verts[(j + 1) % flist[i]->nverts]];

            nx += (current->y - next->y) * (current->z + next->z);
            ny += (current->z - next->z) * (current->x + next->x);
            nz += (current->x - next->x) * (current->y + next->y);
        }

        float length = sqrtf(powf(nx, 2) + powf(ny, 2) + powf(nz, 2));

        nx /= length;
        ny /= length;
        nz /= length;

        // add surface normal vector to vertex normal vector
        for (int j = 0; j < flist[i]->nverts; j++) {
            vlist[flist[i]->verts[j]]->nx += nx;
            vlist[flist[i]->verts[j]]->ny += ny;
            vlist[flist[i]->verts[j]]->nz += nz;
        }
    }

    // normalize vertex normal vectors
    for (int i = 0; i < num_verts; i++) {
        float length = sqrtf(powf(vlist[i]->nx, 2) + powf(vlist[i]->ny, 2) + powf(vlist[i]->nz, 2));

        vlist[i]->nx /= length;
        vlist[i]->ny /= length;
        vlist[i]->nz /= length;
    }
}

void init() {    
    // read in ply file
    read_test((char*)FILE_NAME);

	glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glDisable(GL_CULL_FACE);

    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    GLfloat light_ambient[] = { .8, .8, .8, 1.0 };
    GLfloat light_diffuse[] = { .8, .8, .8, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 1.0, 1.0, -1.0, 1.0 };

    //glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    //glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_diffuse[] = { .8, .8, .8, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 100.0 };
    GLfloat mat_emission[] = { 0.0, 0.0, 0.0, 1.0 };

    //glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);

    calc_normals();

    internal_diagonal = sqrtf(powf(min_corner.x - max_corner.x, 2) + powf(min_corner.y - max_corner.y, 2) + powf(min_corner.z - max_corner.z, 2));

    float ratio = (float)WIDTH / HEIGHT;

    if (ratio > 1) {
        x_bound *= ratio;
    }
    else {
        y_bound /= ratio;
    }

    plyIndex = glGenLists(1);
    glNewList(plyIndex, GL_COMPILE);
    for (int i = 0; i < num_elems; i++) {
        glBegin(GL_POLYGON);
        // color each face based on average of all vertices faces
        float r = 0, g = 0, b = 0;

        for (int j = 0; j < flist[i]->nverts; j++) {
            Vertex* v = vlist[flist[i]->verts[j]];
            // color
            r += (v->x - min_corner.x) / (max_corner.x - min_corner.x) / flist[i]->nverts;
            g += (v->y - min_corner.y) / (max_corner.y - min_corner.y) / flist[i]->nverts;
            b += (v->z - min_corner.z) / (max_corner.z - min_corner.z) / flist[i]->nverts; 
        }

        glColor3f(r, g, b);
        //glColor3f(1, 1, 1);
        // draw vertices at that position
        for (int j = 0; j < flist[i]->nverts; j++) {
            Vertex* v = vlist[flist[i]->verts[j]];
            glNormal3f(v->nx, v->ny, v->nz);
            glVertex3f(v->x, v->y, v->z);
        }
        glEnd();
    }
    glEndList();
}

void displayPly(int display_x, int display_y, int display_width, int display_height, float cam_x, float cam_y, float cam_z, float up_x, float up_y, float up_z) {
    
    glMatrixMode(GL_MODELVIEW);
    glViewport(display_x, display_y, display_width, display_height);
    glLoadIdentity();

    //set camera pos to (0, 0, 2) looking at origin
    gluLookAt(cam_x, cam_y, cam_z, 0, 0, 0, up_x, up_y, up_z);

    // normalize object in region bounded by (-1, -1, -1) and (1, 1, 1)
    // scaling by internal diagonal so that no matter what I rotate it by, it should be within the visible region
    glScalef(2 / internal_diagonal, 2 / internal_diagonal, 2 / internal_diagonal);

    // rotate 
    //glRotatef(angle, axis_x, axis_y, axis_z);
    glRotatef(s1.getVal(), 1, 0, 0);
    glRotatef(s2.getVal(), 0, 1, 0);
    glRotatef(s3.getVal(), 0, 0, 1);

    // translate to center to origin
    glTranslatef(-(min_corner.x + max_corner.x) / 2, -(min_corner.y + max_corner.y) / 2, -(min_corner.z + max_corner.z) / 2);
    
    //iterate through all faces

    glCallList(plyIndex);
}

void display() {

    double currentTime = glutGet(GLUT_ELAPSED_TIME);
    nbFrames++;
    double dt = (currentTime - lastTime) / 1000;
    if (dt >= 1.0) { 
        std::cout << nbFrames / dt << std::endl;
        nbFrames = 0;
        lastTime = currentTime;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int view_width = WIDTH / 2;
    int view_height = HEIGHT / 2;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-x_bound, x_bound, -y_bound, y_bound, 1, 3);

    // display the three different views
    displayPly(view_width, view_height, view_width, view_height, -2, 0, 0, 0 , 1, 0);
    displayPly(0, view_height, view_width, view_height, 0, 2, 0, 1, 0, 0);
    displayPly(0, 0, view_width, view_height, 0, 0, 2, 0, 1, 0);

    //mouse input stuff
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, WIDTH, HEIGHT);
    glLoadIdentity();
    glColor3f(1, 0, 0);
    s1.draw();
    s2.draw();
    s3.draw();

	glFlush();
}

void mouseClicked(int button, int state, int x, int y) {
    s1.mouseClicked(button, state, x, HEIGHT - y);
    s2.mouseClicked(button, state, x, HEIGHT - y);
    s3.mouseClicked(button, state, x, HEIGHT - y);
    glutPostRedisplay();
}

void mouseDragged(int x, int y) {
    s1.mouseDragged(x, y);
    s2.mouseDragged(x, y);
    s3.mouseDragged(x, y);
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Assignment 2");
	init();
	glutDisplayFunc(display);
    glutMouseFunc(mouseClicked);
    glutMotionFunc(mouseDragged);
	glutMainLoop();

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
