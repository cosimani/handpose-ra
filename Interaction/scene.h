#ifndef SCENE_H
#define SCENE_H

#define RESOLUTION_WIDTH  640
#define RESOLUTION_HEIGHT 480

#include <stdio.h>
#include <stdlib.h>

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QVector>
#include <QGLWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGLFunctions>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <aruco/aruco.h>

#include "texture.h"
#include "model.h"
#include "video.h"

#include "principal.h"

#include <QSlider>

using namespace cv;
using namespace std;
using namespace aruco;

class Skin : public QObject
{
    Q_OBJECT

public:

    int minimumHue;
    int maximumHue;
    int minimumSat;
    int maximumSat;
    int minimumVal;
    int maximumVal;

    Skin( QObject *parent = 0 ) : QObject( parent ),
                                  minimumHue( -1 ),
                                  maximumHue( -1 ),
                                  minimumSat( -1 ),
                                  maximumSat( -1 ),
                                  minimumVal( -1 ),
                                  maximumVal( -1 )
    {
    }

    void addMinHue( int hue )  {
        minimumHue = hue;
    }
    void addMaxHue( int hue )  {
        maximumHue = hue;
    }

    void addGoodValue( int hue, int sat, int val )
    {
        if( minimumHue < 0 || maximumHue < 0 )
        {
            minimumHue = hue; maximumHue = hue;
        }

        if( minimumSat < 0 || maximumSat < 0 )
        {
            minimumSat = sat; maximumSat = sat;
        }

        if( minimumVal < 0 || maximumVal < 0 )
        {
            minimumVal = val; maximumVal = val;
        }

        if( hue < minimumHue ) minimumHue = hue;
        if( hue > maximumHue ) maximumHue = hue;

        if( sat < minimumSat ) minimumSat = sat;
        if( sat > maximumSat ) maximumSat = sat;

        if( val < minimumVal ) minimumVal = val;
        if( val > maximumVal ) maximumVal = val;
    }
};

class Scene : public QGLWidget, protected QGLFunctions
{
    Q_OBJECT

private:

    int device;
    VideoCapture *videoCapture;

    QTimer *sceneTimer;
    bool videoActive;

    QVector< Texture * > *textures;
    QVector< Model * > *models;
    QVector< Video * > *videos;

    CameraParameters *cameraParameters;

    Skin *refSkin;
    Point baricenter;

    vector< Point > relevants;
    int textureIndex, modelIndex;
    int lastFingers;

    double distance( Point a, Point b );

    vector< float > matrix;
    void calculateMatrix();

    void loadTextures();
    void loadModels();
    void prepareModels();
    void loadTexturesForModels();
    void loadVideos();

    void process( Mat &frame );

    void drawCamera( int percentage = 100 );
    void drawCameraBox( int percentage = 100 );
    void drawSheet( QString textureName, int percentage = 100 );
    void drawBox( QString textureName, int percentage = 100 );
    void drawModel( QString modelName, int percentage = 100 );
    void drawVideo( QString videoName, int volume = 100, int percentage = 100 );
    void decreaseVideoVolume( QString videoName );

    friend void Principal::slot_algunSliderModificado();

    float y, z;
    int rotacion;

public:

    Scene( QWidget *parent = 0 );

protected:

    void initializeGL();
    void resizeGL( int width, int height );
    void paintGL();

    void keyPressEvent( QKeyEvent *event );
    void mouseMoveEvent( QMouseEvent *event );

private slots:

    void slot_updateScene();

signals:

    void message( QString text );
};

#endif // SCENE_H
