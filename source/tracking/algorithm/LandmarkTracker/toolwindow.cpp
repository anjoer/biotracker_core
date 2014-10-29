#include "toolwindow.h"
#include <iostream>
#include "LandmarkTracker.h"


ToolWindow::ToolWindow(LandmarkTracker *parentTracker, QWidget *parent) :
	QDialog(parent), tracker(parentTracker),
    ui(new Ui::ToolWindow)
{

	ui->setupUi(this);

	QObject::connect(ui->pushButton, SIGNAL(  clicked() ), this, SLOT(emitClose()));
	setWindowFlags(Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint);
    
	//std::cout<<"DOING STUFF HERE!!!!"<<std::endl;
   
}

ToolWindow::~ToolWindow()
{
    //delete ui;
}

void ToolWindow::initToolWindow()
{

	roiMat = tracker->getSelectedRoi();

	std::cout<<"First ROI loaded..."<<std::endl;
	ui->roiOne->setPixmap(Mat2QPixmap(roiMat).scaled(ui->roiOne->size(),Qt::KeepAspectRatio, Qt::FastTransformation));
 
    getRGBValues(roiMat);
}

QPixmap ToolWindow::Mat2QPixmap(const Mat &mat)
{
    Mat rgb;
    QPixmap p;
	
    cvtColor(mat, rgb, CV_BGR2RGB);
    p.convertFromImage(QImage((const unsigned char*)(rgb.data), rgb.cols, rgb.rows, QImage::Format_RGB888));

    std::cout<<"Pixmap set..."<<std::endl;

    return p;
}

//Ausgabe f�r Vector
std::ostream &operator<<(std::ostream &os, const Vec3b &v)
{
	return os<<"("<<static_cast<unsigned>(v.val[0])<<", "<<static_cast<unsigned>(v.val[1])<<", "<<static_cast<unsigned>(v.val[2])<<")";
}

void ToolWindow::getRGBValues(const Mat &mat)
{

    Mat image = mat;

    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
			rgbMap[image.at<Vec3b>(i, j)]++;
        }
    }

	std::cout<<"rgbMap Size: "<<rgbMap.size()<<std::endl;
	
	/*for(const auto &v:rgbMap){
		std::cout<<"Vector: "<<v.first<< "| "<<v.second<<std::endl;
	}*/
   
    std::cout <<"RGB VALUES COMPUTED!"<<std::endl;

}

void ToolWindow::emitClose()
{
	this->close();
	tracker->setToolPtr();
}


