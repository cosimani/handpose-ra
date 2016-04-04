#include "scene.h"
#include <QApplication>

Scene::Scene( QWidget *parent ) : QGLWidget( parent ),
                                  device( 1 ),
                                  videoCapture ( new cv::VideoCapture( device ) ),

                                  sceneTimer ( new QTimer ),
                                  videoActive( false ),

                                  textures( new QVector< Texture * > ),
                                  models( new QVector< Model * > ),
                                  videos( new QVector< Video * > ),

                                  cameraParameters( new CameraParameters ),

                                  refSkin( new Skin( this ) ),

                                  textureIndex( 0 ), modelIndex(0),
                                  lastFingers( 0 ),

                                  y(0), z(0), rotacion(0)
{
    this->setFixedSize( videoCapture->get( CV_CAP_PROP_FRAME_WIDTH ), videoCapture->get( CV_CAP_PROP_FRAME_HEIGHT ) );

    cameraParameters->readFromXMLFile( "../Files/CameraParameters.yml" );

    sceneTimer->start( 10 );
    connect( sceneTimer, SIGNAL( timeout() ), SLOT( slot_updateScene() ) );

}

double Scene::distance( Point a, Point b )
{
    return ( a.x - b.x ) * ( a.x - b.x ) + ( a.y - b.y ) * ( a.y - b.y );
}

void Scene::calculateMatrix()
{
    matrix.clear();

    switch( relevants.size() )
    {
        case 3:
        {
            break;
        }
        case 6:
        {
            break;
        }
        case 9:
        {
            break;
        }
        case 12:
        {
            matrix.push_back( relevants.at( 10 ).x - baricenter.x );
            matrix.push_back( baricenter.y - relevants.at( 10 ).y );
            matrix.push_back( 0 );
            matrix.push_back( relevants.at( 9 ).x - baricenter.x );
            matrix.push_back( baricenter.y - relevants.at( 9 ).y );
            matrix.push_back( 0 );
            matrix.push_back( relevants.at( 2 ).x - baricenter.x );
            matrix.push_back( baricenter.y - relevants.at( 2 ).y );
            matrix.push_back( 0 );
            matrix.push_back( relevants.at( 1 ).x - baricenter.x );
            matrix.push_back( baricenter.y - relevants.at( 1 ).y );
            matrix.push_back( 0 );
            break;
        }
    }

    for( unsigned int i = 0; i < matrix.size(); i++ )
        matrix.operator []( i ) /= 5000.0;
}

void Scene::loadTextures()
{
    QDir directory( "../Textures" );
    QStringList fileFilter;
    fileFilter << "*.jpg" << "*.png" << "*.bmp" << "*.gif";
    QStringList imageFiles = directory.entryList( fileFilter );

    for ( int i = 0; i < imageFiles.size(); i++ )
    {
        textures->append( new Texture( imageFiles.at( i ) ) );
        QString textureUri = "../Textures/" + imageFiles.at( i );

        Mat textureMat = imread( textureUri.toStdString() );
        flip( textureMat, textureMat, 0 );
        textures->last()->mat = textureMat;
        textures->last()->generateFromMat();
    }
}

void Scene::loadModels()
{
    QDir directory( "../Models" );
    QStringList fileFilter;
    fileFilter << "*.3ds";
    QStringList modelFiles = directory.entryList( fileFilter );

    for ( int i = 0 ; i < modelFiles.size() ; i++ )
        models->append( new Model( modelFiles.at( i ) ) );

    prepareModels();
}

void Scene::prepareModels()
{
    loadTexturesForModels();

    for ( int i = 0 ; i < models->size() ; i++)
    {
        if( !models->at( i ) ) return;

        models->at( i )->getFaces();
        Lib3dsVector *vertices = new Lib3dsVector[ models->at( i )->totalFaces * 3 ];
        Lib3dsVector *normals = new Lib3dsVector[ models->at( i )->totalFaces * 3 ];
        Lib3dsTexel *texCoords = new Lib3dsTexel[ models->at( i )->totalFaces * 3 ];
        Lib3dsMesh *mesh;

        unsigned int finishedFaces = 0;

        for( mesh = models->at(i)->model->meshes; mesh != NULL ; mesh = mesh->next )
        {
            lib3ds_mesh_calculate_normals( mesh, &normals[ finishedFaces * 3 ] );
            for( unsigned int currentFace = 0; currentFace < mesh->faces ; currentFace++ )
            {
                Lib3dsFace * face = &mesh->faceL[ currentFace ];
                for( unsigned int i = 0; i < 3; i++ )
                {
                    if( &mesh->texelL )
                        memcpy( &texCoords[ finishedFaces*3 + i ],
                                mesh->texelL[ face->points[ i ] ],
                                sizeof( Lib3dsTexel ) );

                    memcpy( &vertices[ finishedFaces * 3 + i ],
                            mesh->pointL[ face->points[ i ] ].pos,
                            sizeof( Lib3dsVector ) );
                }
                finishedFaces++;
            }
        }

        glGenBuffers( 1, &models->at(i)->vertexVBO );
        glBindBuffer( GL_ARRAY_BUFFER, models->at(i)->vertexVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( Lib3dsVector ) * 3 * models->at(i)->totalFaces, vertices, GL_STATIC_DRAW );

        glGenBuffers( 1, &models->at(i)->normalVBO );
        glBindBuffer( GL_ARRAY_BUFFER, models->at(i)->normalVBO);
        glBufferData( GL_ARRAY_BUFFER, sizeof( Lib3dsVector ) * 3 * models->at(i)->totalFaces, normals, GL_STATIC_DRAW );

        glGenBuffers( 1, &models->at(i)->texCoordVBO );
        glBindBuffer( GL_ARRAY_BUFFER, models->at(i)->texCoordVBO);
        glBufferData( GL_ARRAY_BUFFER, sizeof( Lib3dsTexel ) * 3 * models->at(i)->totalFaces, texCoords, GL_STATIC_DRAW );

        delete vertices;
        delete normals;
        delete texCoords;

        lib3ds_file_free( models->at(i)->model );
        models->at(i)->model = NULL;
    }
}

void Scene::loadTexturesForModels()
{
    for ( int i = 0 ; i < models->size(); i++ )
    {
        QString modelTextureName = models->at( i )->name;
        modelTextureName.remove( ".3ds" );
        modelTextureName += ".jpg";

        for( int j = 0; j < textures->size(); j++ )
            if( textures->at( j )->name == modelTextureName )
                models->operator []( i )->textureId = textures->at( j )->id;
    }
}

void Scene::loadVideos()
{
    QDir directory( "../Videos" );
    QStringList fileFilter;
    fileFilter << "*.avi" << "*.aac" << "*.wmv" << "*.mpg" << "*.mpeg" << "*.mpeg1" << "*.mpeg2" << "*.mpeg4" << "*.mp4";
    QStringList videoFiles = directory.entryList( fileFilter );

    for ( int i = 0 ; i < videoFiles.size() ; i++ )
        videos->append( new Video( videoFiles.at( i ) ) );
}

void Scene::initializeGL()
{
    initializeGLFunctions();

    glClearColor( 0, 0, 0, 0 );
    glShadeModel( GL_SMOOTH );
    glEnable( GL_DEPTH_TEST );

    GLfloat lightAmbient[4]; lightAmbient[0] = 0.5f;  lightAmbient[1] = 0.5f;
            lightAmbient[2] = 0.5f;  lightAmbient[3] = 1.0f;

    GLfloat lightDiffuse[4]; lightDiffuse[0] = 1.0f;  lightDiffuse[1] = 1.0f;
            lightDiffuse[2] = 1.0f;  lightDiffuse[3] = 1.0f;

    GLfloat lightPosition[4];lightPosition[0]= 0.0f;  lightPosition[1]= 0.0f;
            lightPosition[2]= 2.0f;  lightPosition[3]= 1.0f;

    glLightfv( GL_LIGHT1, GL_AMBIENT, lightAmbient );  glLightfv( GL_LIGHT1, GL_DIFFUSE, lightDiffuse );
    glLightfv( GL_LIGHT1, GL_POSITION,lightPosition ); glEnable( GL_LIGHT1 );

    textures->append( new Texture( "CameraTexture" ) );

    loadTextures();
    emit message( "Texturas cargadas" );

    loadModels();
    emit message( "Modelos cargadas" );

//    Resolver: No existe un decodificador disponible...
//    loadVideos();
//    emit message( "Videos cargados" );
}

void Scene::resizeGL( int width, int height )
{
    glViewport( 0, 0, width, height );
}

void Scene::paintGL()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glOrtho( 0, RESOLUTION_WIDTH, 0, RESOLUTION_HEIGHT, 1, 1000 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // Inicio: Gráfico de cámara

    glEnable( GL_TEXTURE_2D );
    glColor3f( 1, 1, 1 );
    glBindTexture( GL_TEXTURE_2D, textures->at( 0 )->id );
    glBegin( GL_QUADS );

        glTexCoord2f( 0, 0 ); glVertex3f( 0, RESOLUTION_HEIGHT, -999 );
        glTexCoord2f( 1, 0 ); glVertex3f( RESOLUTION_WIDTH, RESOLUTION_HEIGHT, -999 );
        glTexCoord2f( 1, 1 ); glVertex3f( RESOLUTION_WIDTH, 0, -999 );
        glTexCoord2f( 0, 1 ); glVertex3f( 0, 0, -999 );

    glEnd();
    glDisable( GL_TEXTURE_2D );

    // Fin: Gráfico de cámara

    glMatrixMode( GL_PROJECTION );
    double projectionMatrix[16];

    cv::Size2i sceneSize( RESOLUTION_WIDTH, RESOLUTION_HEIGHT );
    cv::Size2i openGlSize( RESOLUTION_WIDTH, RESOLUTION_HEIGHT );
    cameraParameters->glGetProjectionMatrix( sceneSize, openGlSize, projectionMatrix, 0.05, 10 );

    glLoadMatrixd( projectionMatrix );
    glMatrixMode( GL_MODELVIEW );
    double modelview_matrix[16];

    // Inicio: Graficos sobre la mano abierta

    if( matrix.size() == 12 && relevants.size() == 12 )
    {
        vector< Point2f > corners;
        corners.clear();

        corners.push_back( relevants.at( 10 ) );
        corners.push_back( relevants.at( 9 ) );
        corners.push_back( relevants.at( 2 ) );
        corners.push_back( relevants.at( 1 ) );

        Marker marker( corners, 1 );

        marker.calculateExtrinsicsHandMatrix( 0.08f,
                                              cameraParameters->CameraMatrix,
                                              matrix,
                                              cameraParameters->Distorsion,
                                              true );

        marker.glGetModelViewMatrix( modelview_matrix );
        glLoadMatrixd( modelview_matrix );

        // Dibuja imagenes planas
        glTranslatef( 0.005, y, z );
        glRotatef( rotacion, 1, 0, 0 );

//        drawSheet( ( textures->at( textureIndex )->name ), 35 );
        drawBox( ( textures->at( textureIndex )->name ), 20 );
//        drawModel( ( models->at( modelIndex )->name ), 8 );
//        drawVideo( "trailer-RF7.mp4", 100, 200 );

    }

    // Fin: Graficos sobre la mano abierta

    glFlush();
}

void Scene::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )  {
    case Qt::Key_Tab:
        if( device ) device = 0;
        else device = 1;

        videoCapture->release();
        videoCapture->open( device );
        break;

    case Qt::Key_C:
        this->calculateMatrix();
        break;

    case Qt::Key_Escape:
        qApp->quit();
        break;

    case Qt::Key_Up:
        z -= 0.001;
        break;

    case Qt::Key_Down:
        z += 0.001;
        break;

    case Qt::Key_Left:
//        y -= 0.001;
        rotacion += 5;
        break;

    case Qt::Key_Right:
//        y += 0.001;
        rotacion -= 5;
        break;

    default:;
    }



}

void Scene::mouseMoveEvent( QMouseEvent *event )
{
    Mat hsvFrame;
    cvtColor( textures->at( 0 )->mat, hsvFrame, CV_BGR2HSV );

    Vec3b color = hsvFrame.at< Vec3b >( Point( event->x(), event->y() ) );

    this->refSkin->addGoodValue( color[0], color[1], color[2] );
}

void Scene::process( Mat &frame )
{
    // Filtramos por color

    Mat hsvFrame;
//    cvtColor( frame, hsvFrame, CV_BGR2HSV );  // Del Emi
    cvtColor( frame, hsvFrame, CV_BGR2Lab );  // Del Cesar

    std::vector< uint8_t > mask;

    for( int j = 0; j < hsvFrame.rows; j++ )
    {
        for( int i = 0; i < hsvFrame.cols; i++ )
        {
            Vec3b color = hsvFrame.at< Vec3b >( Point( i, j ) );

// Esto es del Emi que elige hue, sat y val haciendo clic en la pantalla
//            if( color[0] >= refSkin->minimumHue && color[0] <= refSkin->maximumHue &&
//                color[1] >= refSkin->minimumSat && color[1] <= refSkin->maximumSat &&
//                color[2] >= refSkin->minimumVal && color[2] <= refSkin->maximumVal )
//            {
//                mask.push_back( 255 );
//            }
//            else
//            {
//                mask.push_back( 0 );
//            }

            // Yo solo controlo el a del Lab sacado de los QSlider
            if( color[1] >= refSkin->minimumHue && color[1] <= refSkin->maximumHue )
            {
                mask.push_back( 0 );
            }
            else
            {
                mask.push_back( 255 );
            }

        }
    }

    Mat binary( Size( hsvFrame.cols, hsvFrame.rows ), CV_8UC1, ( void* )mask.data() );

    // Erosion y dilatacion de la imagen binaria

//    Mat matrix = ( Mat_< uchar >( 7, 7 ) << 0,0,1,1,1,0,0,
//                                            0,1,1,1,1,1,0,
//                                            1,1,1,1,1,1,1,
//                                            1,1,1,1,1,1,1,
//                                            1,1,1,1,1,1,1,
//                                            0,1,1,1,1,1,0,
//                                            0,0,1,1,1,0,0);

    int erosion_size = 9;
    Mat matrix = getStructuringElement( MORPH_CROSS,
                                        Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                                        Point( erosion_size, erosion_size ) );


    erode( binary, binary, matrix );
    dilate( binary, binary, matrix );

    Mat binaryCopy = binary.clone();

    // Cierre convexo

    vector< Point > points;

    for( int i = 0; i < binary.rows; i++ )
    {
        for( int j = 0; j < binary.cols; j++ )
        {
            if( ( ( int )binary.at< uchar >( i, j ) ) == 255 )
            {
                points.push_back( Point( j, i ) );
            }
        }
    }

    vector< Point > hull;
    hull.clear();

    convexHull( Mat( points ), hull, false );

    // Dibujamos extremos

    int minimumDistance = 3000;

    vector< Point > baricenters;

    if( hull.size() > 1 )
    {
        if( distance( hull.at( 0 ), hull.at( ( hull.size() - 1 ) ) ) > minimumDistance )
        {
            baricenters.push_back( hull.at( 0 ) );
        }
    }

    for( unsigned int i = 1; i < hull.size(); i++ )
    {
        if( distance( hull.at( i ), hull.at( i - 1 ) ) > minimumDistance )
        {
            baricenters.push_back( hull.at( i ) );
        }
    }

    // Centro de masa segun cierre convexo

    if( baricenters.size() > 0 )
    {
        baricenter.x = 0;
        baricenter.y = 0;

        for( unsigned int i = 0; i < baricenters.size(); i++ )
        {
            baricenter.x += baricenters.at( i ).x;
            baricenter.y += baricenters.at( i ).y;
        }

        baricenter.x /= baricenters.size();
        baricenter.y /= baricenters.size();

        // Dibuja el centro de la palma
//        circle( frame, baricenter, 10 , Scalar( 255, 0, 255 ), 2 );

        // Dibuja lineas grises desde el centro a los bordes
        for( unsigned int i = 0; i < baricenters.size(); i++ )
        {
            line( frame, baricenters.at( i ), baricenter, Scalar( 128, 128, 128 ), 1 );
        }
    }

    // Dibujamos bordes, Entre las puntas de los dedos
    if( hull.size() > 1 )
        line( frame, hull.at( 0 ), hull.at( ( hull.size() - 1 ) ), Scalar( 128, 128, 128 ), 1 );

    double distanciaEuclidian = 0;
    double sumatoriaDistancia = 0;
    double sumatoriaVarianzaDistancia = 0;
    float promediaDistancia = 0;
    float varianzaDistancia = 0;

    // Dibujamos bordes, Entre las puntas de los dedos
    for( unsigned int i = 1; i < hull.size(); i++ )
    {
        line( frame, hull.at( i ), hull.at( i - 1 ), Scalar( 128, 128, 128 ), 1 );

        distanciaEuclidian = cv::norm(hull.at( i ) - hull.at( i - 1 ));//Euclidian distance
        sumatoriaDistancia += distanciaEuclidian;
//        qDebug() << "distanciaEuclidian=" << distanciaEuclidian;

    }

    promediaDistancia = sumatoriaDistancia/(hull.size()-1);
//    qDebug() << "promediaDistancia=" << promediaDistancia;

    for( unsigned int i = 1; i < hull.size(); i++ )
    {

        distanciaEuclidian = cv::norm(hull.at( i ) - hull.at( i - 1 ));//Euclidian distance
        sumatoriaVarianzaDistancia = sumatoriaVarianzaDistancia +
                (distanciaEuclidian-promediaDistancia) * (distanciaEuclidian-promediaDistancia);
//        qDebug() << "sumatoriaVarianzaDistancia=" << sumatoriaVarianzaDistancia;

    }

    varianzaDistancia = sqrt(sumatoriaVarianzaDistancia/(hull.size()-1));
    qDebug() << "varianzaDistancia = " << varianzaDistancia;


    // Buscamos los contornos en la imagen binaria
    vector< vector< Point > > contours;
    findContours( binary, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE );

    relevants.clear();
    int fingers = 1;

    for( unsigned int i = 0 ; i < contours.size(); i++ )
    {
        // Ignoramos las areas insignificantes
        if( contourArea( contours[i] ) >= 3000 )
        {
            // Detectamos cierre convexo en el contorno actual
            vector<vector< Point > > hulls( 1 );
            vector<vector< int > > hullsI( 1 );
            convexHull( Mat( contours[i] ), hulls[0], false );
            convexHull( Mat( contours[i] ), hullsI[0], false );

            // Buscamos defectos de convexidad
            vector< Vec4i > defects;
            if ( hullsI[0].size() > 0 )
            {
                convexityDefects( contours[i], hullsI[0], defects );
                if( defects.size() >= 3 )
                {
                    qDebug() << "\n==============================";

                    float umbral = 0;  // umbral para la profundidad de la concavidad
                    float sumatoria = 0;
                    float promedio = 0;
                    float varianza = 0;

                    // Para el promedio
                    for( unsigned int j = 0; j < defects.size(); j++ )
                    {
                        float depth = defects[j][3] / 256;

                        sumatoria += depth;
                    }
                    promedio = sumatoria/defects.size();
//                    qDebug() << "Promedio = " << promedio;

                    sumatoria = 0;  // Ponemos en cero para usarla para la varianza

                    // Para la varianza
                    for( unsigned int j = 0; j < defects.size(); j++ )
                    {
                        float depth = defects[j][3] / 256;
                        sumatoria = sumatoria + (depth-promedio) * (depth-promedio);
                    }

                    varianza = sqrt(sumatoria/defects.size());
//                    qDebug() << "Varianza = " << varianza;

                    for( unsigned int j = 0; j < defects.size(); j++ )
                    {
                        // the farthest from the convex hull point within the defect
                        float depth = defects[j][3] / 256;



//                        qDebug() << depth;

                        // Filtramos por profundidad
//                        if( depth > 100 )  // Umbral para que no detecte valle en la muneca
//                        if( depth > 70 )
                        if( depth > promedio )
                        {
                            // Entra a este if cada vez que supere esta depth, deberia detectar 4 depth que superen
                            // este umbral. Es decir, detectar los 4 valles. Si el antebrazo se muestra mucho,
                            // entonces quizas se detecte un valle mas, que parezca un sexto dedo.

                            // Cuando la mano se aleja, los depth no llegan a superar el umbral. Deberia fijarse
                            // como umbral un porcentaje y no una distancia fija.

                            // Valles = convexity defects = defectos de convexidad
                            // Envoltura convexa = convex hull

                            int startidx = defects[j][0];
                            Point ptStart( contours[i][startidx] );

                            int endidx = defects[j][1];
                            Point ptEnd( contours[i][endidx] );

                            int faridx = defects[j][2];
                            Point ptFar( contours[i][faridx] );

//                            qDebug() << "cv::norm(ptStart - ptEnd)"  << cv::norm(ptStart - ptEnd);


//                            if ( cv::norm(ptStart - ptEnd) < varianzaDistancia )  {

                                circle( frame, ptStart, 8, Scalar( 255, 0, 0 ), 2 );
                                circle( frame, ptFar, 8, Scalar( 0, 255, 0 ), 2 );
                                circle( frame, ptEnd, 8, Scalar( 0, 0, 255 ), 2 );
//                            }

                            relevants.push_back( ptStart );
                            relevants.push_back( ptFar );
                            relevants.push_back( ptEnd );

                            fingers++;
                        }
                    }
                }
            }
        }
    }

    if( fingers > 1 )
    {
        QString text( "Dedos: " + QString::number( fingers ) );
        putText( frame, text.toStdString(), Point( 10, 30 ), 1, 2, Scalar( 255, 0, 0 ) );
    }

    // Aca se detecta la interaccion para cambiar de modelo a dibujar
    if( fingers == 5  && lastFingers == 4 )
    {
        textureIndex++;
        modelIndex++;

        if( textureIndex >= textures->size() ) textureIndex = 0;
        if( modelIndex >= models->size() ) modelIndex = 0;
    }

    lastFingers = fingers;

    // Mostramos miniatura
    Mat preview( binaryCopy.rows, binaryCopy.cols, CV_8UC3 );
    cvtColor( binaryCopy, preview, CV_GRAY2BGR );
    Mat previewResized( 96, 128, CV_8UC3 );
    cv::resize( preview, previewResized, previewResized.size(), 0, 0, INTER_CUBIC );
    previewResized.copyTo( frame( Rect( frame.cols - 135, frame.rows - 103, 128, 96 ) ) );
}

void Scene::drawCamera( int percentage )
{
    drawSheet( "CameraTexture", percentage );
}

void Scene::drawCameraBox( int percentage )
{
    drawBox( "CameraTexture", percentage );
}

void Scene::drawSheet( QString textureName, int percentage )
{
    for( int i = 0; i < textures->size(); i++ )
    {
        if( textures->at( i )->name == textureName )
        {
            float sideLength = percentage / ( float )2300;
            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, textures->at( i )->id );
            glColor3f( 1, 1, 1 );
            glRotated( 90, 1, 0, 0 );
            glTranslatef( 0, 0, sideLength );
            glBegin( GL_QUADS );

                glNormal3f( 0.0f, 0.0f,-1.0f);
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f(-sideLength, -sideLength, -sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f(-sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f( sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f( sideLength, -sideLength, -sideLength );

            glEnd();
            glDisable( GL_TEXTURE_2D);
        }
    }
}

void Scene::drawBox( QString textureName, int percentage )
{
    for( int i = 0; i < textures->size(); i++ )
    {
        if( textures->at( i )->name == textureName )
        {
            float sideLength = percentage / ( float )2300;

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, textures->at( i )->id );
            glColor3f( 1, 1, 1 );
            glRotated( 90, 1, 0, 0 );
            glTranslatef( 0, 0, -sideLength );
            glEnable( GL_LIGHTING );
            glBegin( GL_QUADS );

                glNormal3f( 0.0f, 0.0f, 1.0f ); // Frontal
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f(-sideLength, -sideLength,  sideLength );
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f( sideLength, -sideLength,  sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f( sideLength,  sideLength,  sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f(-sideLength,  sideLength,  sideLength );

                glNormal3f( 0.0f, 0.0f,-1.0f ); // Anterior
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f(-sideLength, -sideLength, -sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f(-sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f( sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f( sideLength, -sideLength, -sideLength );

                glNormal3f( 0.0f, 1.0f, 0.0f ); // Superior
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f(-sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f(-sideLength,  sideLength,  sideLength );
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f( sideLength,  sideLength,  sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f( sideLength,  sideLength, -sideLength );

                glNormal3f( 0.0f,-1.0f, 0.0f ); // Inferior
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f(-sideLength, -sideLength, -sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f( sideLength, -sideLength, -sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f( sideLength, -sideLength,  sideLength );
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f(-sideLength, -sideLength,  sideLength );

                glNormal3f( 1.0f, 0.0f, 0.0f ); // Derecha
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f( sideLength, -sideLength, -sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f( sideLength,  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f( sideLength,  sideLength,  sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f( sideLength, -sideLength,  sideLength );

                glNormal3f( -1.0f, 0.0f, 0.0f ); // Izquierda
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f(-sideLength, -sideLength, -sideLength );
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f(-sideLength, -sideLength,  sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f(-sideLength,  sideLength,  sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f(-sideLength,  sideLength, -sideLength );

            glEnd();
            glDisable( GL_LIGHTING );
            glDisable( GL_TEXTURE_2D );
        }
    }
}

void Scene::drawModel( QString modelName, int percentage )
{
    float scale = percentage / ( float )1000;
    for ( int i = 0 ; i < models->size(); i++ )
    {
        if ( models->at( i )->name == modelName )
        {
            if( !models->at( i )->totalFaces ) return;

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, models->at( i )->textureId );
            glScalef( scale, scale, -scale );
            glEnableClientState( GL_VERTEX_ARRAY );
            glEnableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );

                glBindBuffer( GL_ARRAY_BUFFER, models->at( i )->normalVBO );
                glNormalPointer( GL_FLOAT, 0, NULL );
                glBindBuffer( GL_ARRAY_BUFFER, models->at( i )->texCoordVBO );
                glTexCoordPointer( 2, GL_FLOAT, 0, NULL );
                glBindBuffer( GL_ARRAY_BUFFER, models->at( i )->vertexVBO );
                glVertexPointer( 3, GL_FLOAT, 0, NULL );
                glDrawArrays( GL_TRIANGLES, 0, models->at( i )->totalFaces * 3 );

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );

            glDisable( GL_TEXTURE_2D );
        }
    }
}

void Scene::drawVideo( QString videoName, int volume, int percentage )
{
    for ( int i = 0 ; i < videos->size(); i++ )
    {
        if ( videos->at( i )->name == videoName )
        {
            videos->at( i )->player->play();
            videos->at( i )->player->setVolume( volume );

            float sideLength = percentage / ( float )2300;

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, videos->at( i )->grabber->textureId );
            glRotated( 90, 1, 0, 0 );
            glTranslatef( 0, 0, sideLength );
            glColor3f( 1, 1, 1 );
            glBegin( GL_QUADS );

                glNormal3f( 0.0f, 0.0f,-1.0f);
                glTexCoord2f( 1.0f, 0.0f ); glVertex3f(-sideLength*( 16 / ( float )9 ), -sideLength, -sideLength );
                glTexCoord2f( 1.0f, 1.0f ); glVertex3f(-sideLength*( 16 / ( float )9 ),  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 1.0f ); glVertex3f( sideLength*( 16 / ( float )9 ),  sideLength, -sideLength );
                glTexCoord2f( 0.0f, 0.0f ); glVertex3f( sideLength*( 16 / ( float )9 ), -sideLength, -sideLength );

            glEnd();
            glDisable( GL_TEXTURE_2D);
        }
    }
}

void Scene::decreaseVideoVolume( QString videoName )
{
    for ( int i = 0 ; i < videos->size(); i++ )
    {
        if ( videos->at( i )->name == videoName )
        {
            emit message( "Marcador no detectado, el video se pausará" );
            videos->at( i )->player->setVolume( videos->at( i )->player->volume() - 1 );
            if( videos->at( i )->player->volume() <= 0 )
            {
                emit message( "Video pausado" );
                videos->at( i )->player->pause();
                if( videoName == "Ubp.mp4" ) videoActive = false;
            }
        }
    }
}

void Scene::slot_updateScene()
{
    videoCapture->operator >>( textures->operator []( 0 )->mat );
    process( textures->operator []( 0 )->mat );
    textures->operator []( 0 )->generateFromMat();
    this->updateGL();
}
