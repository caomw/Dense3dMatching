#include "pclQviewer.h"
#include "ui_pclQviewer.h"

#include <fstream>
#include <pcl/recognition/distance_map.h>


/////////////////////////////////////////////////////////////////////////////////////
/// Define pclQviewer
/////////////////////////////////////////////////////////////////////////////////////
pclQviewer::pclQviewer (QWidget *parent) :
    QMainWindow (parent),
    ui (new Ui::pclQviewer)
{
    ui->setupUi (this);
    this->setWindowTitle ("PCL Qviewer");

    // Setup the cloud pointer
    cloud.reset (new PointCloudT);
    cloud2.reset(new PointCloudT);

    // The number of points in the cloud
    cloud->points.resize (200);

    // initialize clipping threshold as 1.0m
    ui->clipThreshold->setText("1.0");
    ui->shiftX_val->setText("0.0");
    ui->shiftY_val->setText("0.0");
    ui->shiftZ_val->setText("0.0");
    this->clipThd = 1.0;
    this->shiftPC_X = 0.0;
    this->shiftPC_Y = 0.0;
    this->shiftPC_Z = 0.0;

    // Set up the QVTK window
    viewer.reset (new pcl::visualization::PCLVisualizer ("viewer", false));
    ui->qvtkWidget->SetRenderWindow (viewer->getRenderWindow ());
    viewer->setupInteractor (ui->qvtkWidget->GetInteractor (),
                             ui->qvtkWidget->GetRenderWindow ());
    ui->qvtkWidget->update ();

    // Connect point size slider
    connect (ui->horizontalSlider_p, SIGNAL (valueChanged (int)),
             this, SLOT (pSliderValueChanged (int)));
    viewer->addPointCloud (cloud, "cloud");
    pSliderValueChanged (1);
    movePcSlider(1);

    // connect matching linewidth with slider
    connect (ui->lineWidthSlider, SIGNAL (valueChanged (int)),
             this, SLOT (lineWidthSlider (int)));
    lineWidthSlider(1);

    // set camera position
    viewer->resetCamera ();
    viewer->initCameraParameters();
    viewer->setCameraPosition(0, 0, 0,  -0.00723988,-0.999971, 0.0021689);

    // update viewer
    ui->qvtkWidget->update ();

    // Output message
    ui->outputMsg->appendPlainText(QString("Program start ..."));

    cb_args.ptColor[0] = 0;
    cb_args.ptColor[1] = 255;
    cb_args.ptColor[2] = 0;

    // params setting
    seedPropaParams.colorThd = 0.0;
    seedPropaParams.descrThd = 0.0;
    seedPropaParams.motDist  = 0.02;
    seedPropaParams.propaNumber  = 2;
    seedPropaParams.searchRadius = 0.01;
    seedPropaParams.denseMatches.resize(0);
    seedPropaParams.denseRef.reset(new PointCloudT);
    seedPropaParams.denseMot.reset(new PointCloudT);

    drawObjsParams.lineDrawOn = 0;
    drawObjsParams.lineIdx = 0;
    drawObjsParams.lineWidth = 1.0;
    drawObjsParams.goodMatches = 0;
}

pclQviewer::~pclQviewer ()
{
    delete ui;
}

/////////////////////////////////////////////////////////////////////////////////////
/// Define load point cloud function
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_LoadPC_clicked()
{
    // load *.pcd file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home/jiang/CvData/kinect_textured/", tr("Files (*.pcd)"));
    if(fileName.size()<1)
    {
        return;
    }

    fName = fileName.toStdString();
    pcl::io::loadPCDFile<PointT>(fName, *cloud);
    std::vector<s16> nanIdx;
    pcl::removeNaNFromPointCloud(*cloud, *cloud, nanIdx);

    // Store the point cloud color
    for (size_t i = 0; i < cloud->size(); i++)
    {
        cloudR.push_back(cloud->points[i].r);
        cloudG.push_back(cloud->points[i].g);
        cloudB.push_back(cloud->points[i].b);
    }

    if( cloud->points.size() )
    {
        // display the loaded point cloud filename
        std::string dispFileName = fName.substr(fName.size()-20).c_str();
        dispFileName = "File_1: " + dispFileName;
        ui->fileName_label->setText( dispFileName.c_str());
        dispFileName = fName.substr(fName.size()-20).c_str();
        dispFileName = "Loaded file " + dispFileName + ".";
        ui->outputMsg->appendPlainText( QString(dispFileName.c_str()) );
        char oMsg[200];
        std::sprintf(oMsg, "Size of loaded point cloud: %u.", cloud->points.size());
        ui->outputMsg->appendPlainText( QString(oMsg) );

        // Point selection function description
        ui->outputMsg->appendPlainText( QString("Shift+click to select feature points ...") );
    }else
    {
        ui->outputMsg->appendPlainText( QString("ERROR: Load .pcd file failed ... ") );
    }

    // Activate point selection function
    this->on_getPoint_clicked();

    // update the point cloud viewer
    viewer->updatePointCloud (cloud, "cloud");
    ui->qvtkWidget->update ();
}

void
pclQviewer::on_showCloud_1_clicked()
{
    QString showKeypts = "Show cloud_1";
    QString hideKeypts = "Hide cloud_1";

    if(cloud->points.size()<1)
    {
        return;
    }
    // switch show/hide state to control the visualization of keypts
    if( QString ::compare( showKeypts, ui->showCloud_1->text(), Qt::CaseInsensitive) )
    {
        ui->showCloud_1->setText(showKeypts);
        viewer->removePointCloud("cloud");
        ui->outputMsg->appendPlainText(QString("Cloud_1 is hidden"));
    }else
    {
        ui->showCloud_1->setText(hideKeypts);
        viewer->addPointCloud(cloud,"cloud");
        ui->outputMsg->appendPlainText(QString("Cloud_1 is shown."));
    }
    ui->qvtkWidget->update();
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to add multiple point clouds
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_add_PC_clicked()
{
    // load *.pcd file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home/jiang/CvData/kinect_textured/", tr("Files (*.pcd)"));
    if(fileName.size()<1)
    {
        return;
    }
    this->fName2 = fileName.toStdString();

    cloud2.reset(new PointCloudT);
    pcl::io::loadPCDFile<PointT>(this->fName2, *cloud2);
    std::vector<s16> nanIdx;
    pcl::removeNaNFromPointCloud(*cloud2,*cloud2, nanIdx);

    if( cloud2->points.size() )
    {
        // display the loaded point cloud filename
        std::string dispFileName = this->fName2.substr(this->fName2.size()-20).c_str();
        dispFileName = this->fName2.substr(this->fName2.size()-20).c_str();
        dispFileName =  "File_2:" + dispFileName;
        ui->fileName_label2->setText(dispFileName.c_str());
        dispFileName = this->fName2.substr(this->fName2.size()-20).c_str();
        dispFileName = "Loaded file " + dispFileName + ".";
        ui->outputMsg->appendPlainText( QString(dispFileName.c_str()) );
        char oMsg[200];
        std::sprintf(oMsg, "Size of loaded point cloud: %u.", cloud2->points.size());
        ui->outputMsg->appendPlainText( QString(oMsg) );
        viewer->addPointCloud(cloud2,this->fName2.substr(this->fName2.size()-20).c_str());
        viewer->updatePointCloud (cloud2, this->fName2.substr(this->fName2.size()-20).c_str());
    }else
    {
        ui->outputMsg->appendPlainText( QString("ERROR: Load .pcd file failed ... ") );
    }

    // Activate point selection function
    this->on_getPoint_clicked();

    // update the point cloud viewer
    ui->qvtkWidget->update ();
}

void
pclQviewer::on_showCloud_2_clicked()
{
    QString showKeypts = "Show cloud_2";
    QString hideKeypts = "Hide cloud_2";

    if(cloud2->points.size()<1)
    {
        return;
    }
    // switch show/hide state to control the visualization of keypts
    if( QString ::compare( showKeypts, ui->showCloud_2->text(), Qt::CaseInsensitive) )
    {
        ui->showCloud_2->setText(showKeypts);
        viewer->removePointCloud(this->fName2.substr(this->fName2.size()-20).c_str());
        ui->outputMsg->appendPlainText(QString("Cloud_2 is hidden"));
    }else
    {
        ui->showCloud_2->setText(hideKeypts);
        viewer->addPointCloud(cloud2,this->fName2.substr(this->fName2.size()-20).c_str());
        ui->outputMsg->appendPlainText(QString("Cloud_2 is shown."));
    }
    ui->qvtkWidget->update();
}

/////////////////////////////////////////////////////////////////////////////////////
/// Display the point cloud with color information
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_chkbox_withColor_clicked()
{
    // show point cloud color
    if(ui->chkbox_withColor->checkState()){
        for (size_t i = 0; i < cloud->size(); i++)
        {
            cloud->points[i].r = cloudR[i];
            cloud->points[i].g = cloudG[i];
            cloud->points[i].b = cloudB[i];
        }
    }
    // show grey point cloud
    else
    {
        for (size_t i = 0; i < cloud->size(); i++)
        {
            cloud->points[i].r = 128;
            cloud->points[i].g = 128;
            cloud->points[i].b = 128;
        }
    }

    // update point cloud viewer
    viewer->updatePointCloud (cloud, "cloud");
    ui->qvtkWidget->update ();
}

/////////////////////////////////////////////////////////////////////////////////////
/// change size of point cloud
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::pSliderValueChanged (int value)
{
    // change the size of point cloud according to the value of slider
    viewer->setPointCloudRenderingProperties (pcl::visualization::
                                              PCL_VISUALIZER_POINT_SIZE, value, "cloud");
    ui->qvtkWidget->update ();
}

/////////////////////////////////////////////////////////////////////////////////////
/// Shift point cloud
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::movePcSlider (int value)
{
    //    this->cloud
    //    ui->qvtkWidget->update ();
}

void
pclQviewer::lineWidthSlider(int value)
{
    this->featDescrStr.lineWidth = (f32)value;
    if(featDescrStr.lineDrawOn)
    {
        on_removeLines_clicked();
        on_drawMatches_clicked();
    }
}
/////////////////////////////////////////////////////////////////////////////////////
/// func to voxelize the point cloud
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_getVoxel_clicked()
{

}

/////////////////////////////////////////////////////////////////////////////////////
/// func to start the kinect streaming
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_StartKinect_clicked()
{

}

/////////////////////////////////////////////////////////////////////////////////////
/// define click point event to select feature points on the point cloud
/////////////////////////////////////////////////////////////////////////////////////
f32
distanceL2(PointT p1, PointT p2)
{
    return std::sqrt( std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2) +
                      std::pow(p1.z - p2.z, 2) );
}

void
clickPoint_callback (const pcl::visualization::PointPickingEvent& event, void* args)
{
    struct callback_args* data = (struct callback_args *) args;

    // if clicking event is null, return.
    if (event.getPointIndex () == -1)
    {
        return;
    }

    // initialize point containers
    PointT current_point, previous_point;
    current_point.x  = .0; current_point.y  = .0; current_point.z  = .0;
    previous_point.x = .0; previous_point.y = .0; previous_point.z = .0;

    // waiting for clicking event
    event.getPoint(current_point.x, current_point.y, current_point.z);

    // pushback the selected features
    unsigned int featureNb = data->clicked_points_3d->points.size();
    if(featureNb)
    {
        previous_point = data->clicked_points_3d->points[featureNb-1];
    }

    // avoid multiple selections of same feature
    f32 tree_distance = 100000.0;
    uc8 save_feature  = 1;
    u16 i = featureNb, stop_loop = 50;
    PointT *tree_feature = &data->clicked_points_3d->points[featureNb];
    // variable stop_loop to avoid infinite searching of duplicate points
    while(i-- && stop_loop--)
    {
        --tree_feature;
        // avoid duplicate points
        tree_distance = distanceL2(*tree_feature, current_point);
        if(tree_distance < 10e-6)
        {
            save_feature = 0; break;
        }
    }
    // constrain two neighbor features have distance bigger than 0.01 meter
    float feature_distance = distanceL2( previous_point, current_point );
    if( feature_distance > 0.01 && save_feature)
    {
        data->clicked_points_3d->push_back(current_point);
    }

    // draw clicked points in green:
    PointColor clickedColor (data->clicked_points_3d, data->ptColor[0],
            data->ptColor[1], data->ptColor[2]);
    data->viewerPtr->removePointCloud("selected_features");
    data->viewerPtr->addPointCloud(data->clicked_points_3d, clickedColor, "selected_features");
    data->viewerPtr->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                      10, "selected_features");

    // Output the information of selected features
    char oMsg[200];
    std::sprintf(oMsg, "Selected pt: %f %f %f.", current_point.x,
                 current_point.y, current_point.z);
    data->txtEditor->appendPlainText( QString(oMsg) );
    std::sprintf(oMsg, "# of selected pt: %u.", data->clicked_points_3d->points.size());
    data->txtEditor->appendPlainText( QString(oMsg) );
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to get feature points from cloud
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_getPoint_clicked()
{
    // initialize the feature points
    featurePts.reset(new PointCloudT);
    cb_args.clicked_points_3d = featurePts;

    // Output the number of selected features
    if( cb_args.clicked_points_3d->size() )
    {
        char oMsg[200];
        std::sprintf(oMsg, "Size of selected features: %u.", cb_args.clicked_points_3d->size());
        ui->outputMsg->appendPlainText( QString(oMsg) );
    }
    // point to interface point cloud visualizer
    cb_args.viewerPtr = viewer;
    cb_args.txtEditor = ui->outputMsg;

    // activate clickPoint callback function
    viewer->registerPointPickingCallback (clickPoint_callback, (void*)&cb_args);

    // refresh point cloud viewer
    viewer->updatePointCloud (cloud, "cloud");
    ui->qvtkWidget->update ();
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to save selected features
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_saveFeatures_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    "/home/jiang/CvData/Kinect_MultiObj_Motions/features_.txt", tr("features (*.txt)"));

    std::ofstream ofile;
    // create a new file or select the existing files to continue saving selected features
    ofile.open(fileName.toStdString().c_str(), std::ios_base::app);

    // Data structure: columns are different features; rows are X, Y, Z values
    for(uc8 i=0; i<3; i++)
    {
        PointT *current_point = &featurePts->points[0];
        for(u16 j = 0; j<featurePts->points.size();j++)
        {
            float *dataPt = (*current_point).data;
            ofile << *(dataPt+i) <<" " ;
            ++current_point;
        }
        ofile << std::endl;
    }
    ofile.close();

    pcl::io::savePCDFileASCII ("features_pcd.pcd", *featurePts);

    ui->outputMsg->appendPlainText( QString("Selected features are saved.") );
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to take a screenshot of the point cloud viewer
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_takeScreenshot_clicked()
{
    // take screenshot of active widget
    QPixmap screenshot;
    screenshot = QPixmap::grabWidget(ui->qvtkWidget,ui->qvtkWidget->rect());

    // Name the screenshot according the loaded .pcd file
    std::string ScreenshotName = fName;
    ScreenshotName.erase(ScreenshotName.size()-4);
    ScreenshotName += ".png";

    // save the screenshot
    QFile fileN(ScreenshotName.c_str());
    fileN.open(QIODevice::WriteOnly);
    screenshot.save(&fileN, "PNG");

    ui->outputMsg->appendPlainText( QString("Screenshot is saved.") );
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to clean the selected feature points
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_cleanFeatures_clicked()
{
    // delete the selected feature points
    featurePts->clear();
    viewer->removePointCloud("selected_features");
    ui->qvtkWidget->update ();
    ui->outputMsg->appendPlainText( QString("Selected features deleted.\n") );
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to delete the previous selected feature point
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_delOnePt_clicked()
{
    // pop out the last stored feature
    if(featurePts->points.size()>0)
    {
        featurePts->points.pop_back();

        // update the point cloud
        this->drawKeyPts(featurePts, "selected_features", cb_args.ptColor, 10);
        ui->outputMsg->appendPlainText( QString("Previous selected features deleted.\n") );
    }
    else
    {
        ui->outputMsg->appendPlainText( QString("No selected feature exists.\n") );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/// Get parameters from user setting
/////////////////////////////////////////////////////////////////////////////////////

// Preprocessing
void pclQviewer::on_clipThreshold_editingFinished()
{    this->clipThd = ui->clipThreshold->text().toFloat();                }
void pclQviewer::on_shiftX_val_editingFinished()
{    this->shiftPC_X = ui->shiftX_val->text().toFloat();                 }
void pclQviewer::on_shiftY_val_editingFinished()
{    this->shiftPC_Y = ui->shiftY_val->text().toFloat();                 }
void pclQviewer::on_shiftZ_val_editingFinished()
{    this->shiftPC_Z = ui->shiftZ_val->text().toFloat();                 }


/////////////////////////////////////////////////////////////////////////////////////
/// func to draw points
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::drawKeyPts(const PointCloudT::Ptr &keyPts, const std::string pcName,
                       const uc8 pColor[], const uc8 ptSize)
{
    // color setting
    PointColor color(keyPts, pColor[0], pColor[1], pColor[2]);
    // remove previous point cloud
    viewer->removePointCloud(pcName.c_str());
    // add point cloud
    viewer->addPointCloud(keyPts, color, pcName.c_str());
    // point size setting
    viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                             ptSize, pcName.c_str());
    ui->qvtkWidget->update ();
}

///////////////////////////////////////////////////////////////////////////////////////
///// remove matching lines
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_removeLines_clicked()
{
    if( drawObjsParams.lineDrawOn == 0)
    {
        return;
    }
    for(int idxLine = 0; idxLine<drawObjsParams.lineIdx; ++idxLine)
    {
        QString lineName = QString::number(idxLine);
        viewer->removeShape(lineName.toStdString().c_str());
    }
    drawObjsParams.lineIdx = 0;
    ui->qvtkWidget->update();
    drawObjsParams.lineDrawOn = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to draw matching pairs with weights
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::drawMatches(PointCloudT::Ptr &corr_1, PointCloudT::Ptr & corr_2,
                        std::vector<f32> &matchDist, uc8 viewColor[])
{
    if(corr_1->points.size()<1 | corr_2->points.size()<1 | matchDist.size()<1)
    {   return;     }

    PointT *pt_1 = &corr_1->points.at(0);
    PointT *pt_2 = &corr_2->points.at(0);
    u16 idxLine = 0;
    float maxDist = -1000;
    float minDist = 1000;
    for(u16 i = 0; i<matchDist.size(); i++)
    {
        if(matchDist[i]>maxDist)
        {
            maxDist = matchDist[i];
        }
        if(matchDist[i]<minDist)
        {
            minDist = matchDist[i];
        }
    }

    while( idxLine < corr_1->points.size() )
    {
        QString lineName = QString::number(idxLine);
        // scale the line width with the descriptor distance
        // smaller distance, higher the value
        float scl = minDist/matchDist[idxLine];
        float idxLineWidth = drawObjsParams.lineWidth*scl;
        viewer->addLine(*pt_1, *pt_2, viewColor[0], viewColor[1], viewColor[2],
                lineName.toStdString().c_str());
        viewer->setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_LINE_WIDTH,
                                            idxLineWidth, lineName.toStdString().c_str());

        ++pt_1;
        ++pt_2;
        ++idxLine;
    }

    drawObjsParams.lineIdx = idxLine;
    ui->qvtkWidget->update();
}



/////////////////////////////////////////////////////////////////////////////////////
/// func to load selected features from .pcd files
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_loadSelectedFeat_clicked()
{
    // load *.pcd file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                       "/home/jiang/CvData/kinect_textured/", tr("Files (*.pcd)"));
    if(fileName.size()<1)
    {
        return;
    }
    PointCloudT::Ptr feature_cloud (new PointCloudT);
    std::string featureName = fileName.toStdString();
    pcl::io::loadPCDFile<PointT>(featureName, *feature_cloud);

    std::string cloudName = featureName.substr(featureName.size()-8).c_str();

    // draw clicked points in green:
    PointColor clickedColor (feature_cloud, cb_args.ptColor[0],
            cb_args.ptColor[1], cb_args.ptColor[2]);
    viewer->addPointCloud(feature_cloud, clickedColor, cloudName);
    viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                             15, cloudName);

}

/////////////////////////////////////////////////////////////////////////////////////
/// func to load matching index in two point clouds
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_loadMatchIdx_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home/jiang/bin/PosEstMovObj/", tr("Files (*.txt)"));
    if(fileName.size()<1)
    {
        return;
    }

    std::ifstream readMatchFile;
    readMatchFile.open(fileName.toStdString().c_str());
    featDescrStr.matchIdx1.clear();
    featDescrStr.matchIdx2.clear();
    if (readMatchFile.is_open())
    {
        while (!readMatchFile.eof())
        {
            u16 idx_tmp = 0;
            readMatchFile >> idx_tmp;
            featDescrStr.matchIdx1.push_back(idx_tmp);
            readMatchFile >> idx_tmp;
            featDescrStr.matchIdx2.push_back(idx_tmp);
        }
    }
    readMatchFile.close();
}

/////////////////////////////////////////////////////////////////////////////////////
/// func to transfrom the point cloud, e.g. translation or rotation
/////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_transformPc_clicked()
{
    Eigen::Matrix4f t = Eigen::Matrix4f::Identity();
    t(0,3) = this->shiftPC_X;
    t(1,3) = this->shiftPC_Y;
    t(2,3) = this->shiftPC_Z;

    pcl::transformPointCloud(*cloud, *cloud, t);

    viewer->removePointCloud("cloud");
    viewer->addPointCloud(cloud,"cloud");
    ui->qvtkWidget->update();
}

void
pclQviewer::transformPC(PointCloudT::Ptr &cloudIn,
                        PointCloudT::Ptr &cloudOut,
                        Eigen::Matrix4f &transMat)
{
    pcl::transformPointCloud(*cloudIn, *cloudOut, transMat);
}
///////////////////////////////////////////////////////////////////////////////////////
///// Change plot colors
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_comboBox_activated(int index)
{
    cb_args.ptColor[0] = 0;
    cb_args.ptColor[1] = 0;
    cb_args.ptColor[2] = 0;
    switch(index)
    {
    case 0: // Red
    {   cb_args.ptColor[0] = 255;                             break;}
    case 1: // Green
    {   cb_args.ptColor[1] = 255;                             break;}
    case 2: // Blue
    {   cb_args.ptColor[2] = 255;                             break;}
    case 3: // Cyan
    {   cb_args.ptColor[0] = 255;
        cb_args.ptColor[2] = 255;                             break;}
    case 4: // Magenta
    {   cb_args.ptColor[1] = 255;
        cb_args.ptColor[2] = 255;                             break;}
    case 5: // Black
    {                                                         break;}
    case 6: // White
    {   cb_args.ptColor[0] = 255;
        cb_args.ptColor[1] = 255;
        cb_args.ptColor[2] = 255;                             break;}
    default:
    {   cb_args.ptColor[0] = 255;                             break;}
    }
}

///////////////////////////////////////////////////////////////////////////////////////
///// func to clip the points further than threshold
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_clipPC_clicked()
{
    //    // use clipping to remove far away points
    //    if(cloud->points.size()>1)
    //    {
    //        featureDetector->distClip(cloud, clipThd);
    //        char oMsg[200];
    //        std::sprintf(oMsg, "Cloud 1 points father than %1f meters removed.",
    //                     keyPtsStr.params[11]);
    //        ui->outputMsg->appendPlainText( QString(oMsg) );

    //        // update point cloud
    //        viewer->removePointCloud("cloud");
    //        viewer->addPointCloud(cloud,"cloud");
    //        ui->qvtkWidget->update();
    //    }

    //    if(cloud2->points.size()>1)
    //    {
    //        featureDetector->distClip(cloud2, clipThd);
    //        char oMsg[200];
    //        std::sprintf(oMsg, "Cloud 2 points father than %1f meters removed.",
    //                     keyPtsStr.params[11]);
    //        ui->outputMsg->appendPlainText( QString(oMsg) );
    //        // update point cloud
    //        viewer->removePointCloud(this->fName2.substr(this->fName2.size()-20).c_str());
    //        viewer->addPointCloud(cloud2,this->fName2.substr(this->fName2.size()-20).c_str());
    //        ui->qvtkWidget->update();
    //    }
    ui->outputMsg->appendPlainText( "Clipping is deactivated...");
}

///////////////////////////////////////////////////////////////////////////////////////
///// Save draw dense matching
///////////////////////////////////////////////////////////////////////////////////////
void pclQviewer::on_drawMatches_clicked()
{
    if(seedPropaParams.denseMatches.size()<1){return;}

    if(drawObjsParams.lineIdx != 0)
    {
        ui->removeLines->click();
    }

    std::vector<f32> matchDist;
    triplet<s16,s16,f32> tmpMatch;
    for(int i=0; i<seedPropaParams.denseMatches.size();i++)
    {
        tmpMatch = seedPropaParams.denseMatches.at(i);
        matchDist.push_back(tmpMatch.matchDist);
    }

    f32 medianDist = 0;
    if (drawObjsParams.goodMatches)
    {// draw only half of the good matches
        medianDist = getMedian(matchDist);
    }
    matchDist.clear();
    // get correspondences
    for(int i=0; i<seedPropaParams.denseMatches.size();i++)
    {
        tmpMatch = seedPropaParams.denseMatches.at(i);
        if(!drawObjsParams.goodMatches | tmpMatch.matchDist<medianDist)
        {
            seedPropaParams.denseRef->push_back( cloud->points.at(tmpMatch.idxRef) );
            seedPropaParams.denseMot->push_back( cloud2->points.at(tmpMatch.idxMot) );
            matchDist.push_back(tmpMatch.matchDist);
        }
    }
    drawMatches(seedPropaParams.denseRef, seedPropaParams.denseMot, matchDist, cb_args.ptColor);
    drawObjsParams.lineDrawOn = 1;
}

///////////////////////////////////////////////////////////////////////////////////////
///// assign searching radius for dense matching
///////////////////////////////////////////////////////////////////////////////////////
void pclQviewer::on_denseSearchRadius_editingFinished()
{
    seedPropaParams.searchRadius = ui->denseSearchRadius->text().toFloat();
}
void pclQviewer::on_propaNumber_editingFinished()
{
    seedPropaParams.propaNumber = ui->propaNumber->text().toInt();
}

///////////////////////////////////////////////////////////////////////////////////////
///// show dense matching pairs
///////////////////////////////////////////////////////////////////////////////////////
void pclQviewer::on_showDensePairs_clicked()
{
    QString showMatches = "Show denPairs";
    QString hideMatches = "Hide denPairs";

    if(seedPropaParams.denseRef->points.size()<1 | seedPropaParams.denseRef->points.size()<1)
    {
        return;
    }
    // switch show/hide state to control the visualization of keypts
    if( QString ::compare( showMatches, ui->showDensePairs->text(), Qt::CaseInsensitive) )
    {
        ui->showDensePairs->setText(showMatches);
        viewer->removePointCloud("densePair_1");
        viewer->removePointCloud("densePair_2");
        ui->outputMsg->appendPlainText(QString("Dense matching pairs are hidden"));
    }else
    {
        ui->showDensePairs->setText(hideMatches);
        PointColor densePairColor_1 (seedPropaParams.denseRef, cb_args.ptColor[0],
                cb_args.ptColor[1], cb_args.ptColor[2]);
        viewer->addPointCloud(seedPropaParams.denseRef,densePairColor_1,"densePair_1");
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                 10, "densePair_1");

        PointColor densePairColor_2 (seedPropaParams.denseMot, cb_args.ptColor[1],
                cb_args.ptColor[0], cb_args.ptColor[2]);
        viewer->addPointCloud(seedPropaParams.denseMot,densePairColor_2,"densePair_2");
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                 10, "densePair_2");

        ui->outputMsg->appendPlainText(QString("Dense matching pairs are shown."));

    }
    ui->qvtkWidget->update();
}

///////////////////////////////////////////////////////////////////////////////////////
///// activate dense matching tap
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_dataAnalysisTab_currentChanged(int index)
{
    // dense matching tap is currently in used
    if(index == 1)
    {
        ui->outputMsg->appendPlainText( "Select at least 3 matches \
                                        features for two clouds ...");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
///// Save selected features to perform dense matching
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_saveFeat_1_clicked()
{
    if(featurePts->points.size()<1)
        return;
    // initialize the clickFeat_1 container
    clickFeat_1.reset(new PointCloudT);
    pcl::copyPointCloud(*featurePts, *clickFeat_1);
    on_cleanFeatures_clicked();
    ui->outputMsg->appendPlainText( "Selected features 1 saved.");
}

void
pclQviewer::on_saveFeat_2_clicked()
{
    if(featurePts->points.size()<1)
        return;
    // initialize the clickFeat_1 container
    clickFeat_2.reset(new PointCloudT);
    pcl::copyPointCloud(*featurePts, *clickFeat_2);
    on_cleanFeatures_clicked();
    ui->outputMsg->appendPlainText( "Selected features 2 saved.");
}

///////////////////////////////////////////////////////////////////////////////////////
///// Save selected features to perform dense matching
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_showSeleMatch_clicked()
{
    QString showMatches = "Show Matches";
    QString hideMatches = "Hide Matches";

    if(clickFeat_1->points.size()<1 | clickFeat_2->points.size()<1)
    {
        return;
    }
    // switch show/hide state to control the visualization of keypts
    if( QString ::compare( showMatches, ui->showSeleMatch->text(), Qt::CaseInsensitive) )
    {
        ui->showSeleMatch->setText(showMatches);
        viewer->removePointCloud("selectMatches_1");
        viewer->removePointCloud("selectMatches_2");
        ui->outputMsg->appendPlainText(QString("Selected matches are hidden"));
    }else
    {
        ui->showSeleMatch->setText(hideMatches);
        PointColor clickedColor_1 (clickFeat_1, cb_args.ptColor[0],
                cb_args.ptColor[1], cb_args.ptColor[2]);
        viewer->addPointCloud(clickFeat_1,clickedColor_1,"selectMatches_1");
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                 15, "selectMatches_1");

        PointColor clickedColor_2 (clickFeat_1, cb_args.ptColor[1],
                cb_args.ptColor[0], cb_args.ptColor[2]);
        viewer->addPointCloud(clickFeat_2,clickedColor_2,"selectMatches_2");
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                 15, "selectMatches_2");

        ui->outputMsg->appendPlainText(QString("Selected matches are shown."));

    }
    ui->qvtkWidget->update();
}


///////////////////////////////////////////////////////////////////////////////////////
///// Save selected features to perform dense matching
///////////////////////////////////////////////////////////////////////////////////////
void
pclQviewer::on_denseLocalMatch_clicked()
{
    // create a matcher object
    seedPropagation denseMatcher;
    Eigen::Matrix4f transMat = Eigen::Matrix4f::Identity();

    // rigid transformation of point cloud from sparse matches
    denseMatcher.getTransformMatrix(clickFeat_1, clickFeat_2, transMat);

    // point cloud alignment using inverse of rigid transformation
    pcl::transformPointCloud(*clickFeat_1, *clickFeat_1, transMat);
    pcl::transformPointCloud(*cloud, *cloud, transMat);


    if (0)
    {// Visualize knn neighbors

        std::vector< std::vector<s16> > knnIdxRef, knnIdxMot;
        // get knn neighbors within a Radius
        denseMatcher.getKnnRadius(cloud, clickFeat_1, seedPropaParams.searchRadius, knnIdxRef);
        denseMatcher.getKnnRadius(cloud2, clickFeat_2, seedPropaParams.searchRadius, knnIdxMot);

        PointCloudT::Ptr idxPtsRef (new PointCloudT);
        PointCloudT::Ptr idxPtsMot (new PointCloudT);
        // get the knn neighbors from point cloud
        denseMatcher.copyIdxPtsFromCloud(knnIdxRef[0], cloud, idxPtsRef);
        denseMatcher.copyIdxPtsFromCloud(knnIdxRef[1], cloud, idxPtsRef);
        denseMatcher.copyIdxPtsFromCloud(knnIdxRef[2], cloud, idxPtsRef);
        denseMatcher.copyIdxPtsFromCloud(knnIdxMot[0], cloud2, idxPtsMot);
        denseMatcher.copyIdxPtsFromCloud(knnIdxMot[1], cloud2, idxPtsMot);
        denseMatcher.copyIdxPtsFromCloud(knnIdxMot[2], cloud2, idxPtsMot);

        // show the knn neighbors
        uc8 colorKnn_1[3] = {0, 255, 0}, colorKnn_2[3] = {255, 0, 0};

        drawKeyPts(idxPtsRef,"feat_1", colorKnn_1, 10);
        drawKeyPts(idxPtsMot,"feat_2", colorKnn_2, 10);
    }

    // local dense matching
    denseMatcher.localMatching(cloud, cloud2, clickFeat_1, clickFeat_2, seedPropaParams);

    // shift point cloud to visualize the matching result
    transMat = Eigen::Matrix4f::Identity();
    transMat(2,3) = 0.2;
    pcl::transformPointCloud(*clickFeat_1, *clickFeat_1, transMat);
    pcl::transformPointCloud(*cloud, *cloud, transMat);
    viewer->removePointCloud("cloud");
    viewer->addPointCloud(cloud, "cloud");
    ui->showSeleMatch->click();
    ui->qvtkWidget->update();
}


///////////////////////////////////////////////////////////////////////////////////////
///// show dense matching pairs
///////////////////////////////////////////////////////////////////////////////////////
void pclQviewer::on_propaMatch_clicked()
{
    // create a matcher object
    seedPropagation denseMatcher;
    Eigen::Matrix4f transMat = Eigen::Matrix4f::Identity();

    // rigid transformation of point cloud from sparse matches
    denseMatcher.getTransformMatrix(clickFeat_1, clickFeat_2, transMat);

    // point cloud alignment using inverse of rigid transformation
    pcl::transformPointCloud(*clickFeat_1, *clickFeat_1, transMat);
    pcl::transformPointCloud(*cloud, *cloud, transMat);

    // local dense matching
    denseMatcher.propagateMatching(cloud, cloud2, clickFeat_1, clickFeat_2,
                                   seedPropaParams);

    // shift point cloud to visualize the matching result
    transMat = Eigen::Matrix4f::Identity();
    transMat(2,3) = 0.2;
    pcl::transformPointCloud(*clickFeat_1, *clickFeat_1, transMat);
    pcl::transformPointCloud(*cloud, *cloud, transMat);
    viewer->removePointCloud("cloud");
    viewer->addPointCloud(cloud, "cloud");
    ui->showSeleMatch->click();
    ui->qvtkWidget->update();
}

