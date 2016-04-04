#ifndef TEXTURE_H
#define TEXTURE_H

#include <QGLWidget>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

class Texture : public QObject
{
    Q_OBJECT

public:

    QString name;
    Mat mat;
    GLuint id;

    Texture( QString name = "", QObject *parent = 0 ) : QObject( parent ), name( name ), id( 0 )
    {
        glGenTextures( 1, &id );
    }

    void generateFromMat()
    {
        glBindTexture( GL_TEXTURE_2D, id );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 1, GL_BGR, GL_UNSIGNED_BYTE, mat.data );
    }
};

#endif // TEXTURE_H
