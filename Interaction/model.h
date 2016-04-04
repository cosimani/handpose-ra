#ifndef MODEL_H
#define MODEL_H

#include <QFile>
#include <QGLWidget>
#include <lib3ds/file.h>
#include <lib3ds/mesh.h>

class Model : public QObject
{
    Q_OBJECT

public:

    QString name;
    GLuint textureId;
    int totalFaces;

    Lib3dsFile *model;
    GLuint vertexVBO, normalVBO, texCoordVBO;

    Model( QString name, QObject *parent = 0 ) : QObject( parent ), name ( name ), textureId( 0 ), totalFaces( 0 )
    {
        QString modelUri = "../Models/" + name;
        if( QFile::exists( modelUri ) ) model = lib3ds_file_load( modelUri.toStdString().c_str() );
    }

    virtual void CreateVBO() { }

    virtual ~Model() { if( model != NULL ) lib3ds_file_free( model ); }

    void getFaces()
    {
        if( !model ) return;

        totalFaces = 0;
        Lib3dsMesh * mesh;
        for( mesh = model->meshes; mesh != NULL; mesh = mesh->next ) totalFaces += mesh->faces;
    }
};

#endif // MODEL_H
