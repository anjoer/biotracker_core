#include "BeesBookTagMatcher.h"
#include <QApplication>

#include "source/tracking/algorithm/algorithms.h"

namespace {
    auto _ = Algorithms::Registry::getInstance().register_tracker_type<BeesBookTagMatcher>("BeesBook Tag Matcher");
}

BeesBookTagMatcher::BeesBookTagMatcher(Settings & settings, std::string &serializationPathName, QWidget *parent )
    : TrackingAlgorithm( settings, serializationPathName, parent )
{	
		_ready			= true;  //Ready for a new tag --Ctrl + LCM --
		_setTag			= false; //State while the tag is being set, a red vector is drawn
		_activeTag		= false; //if true, then a new Tag has been defined through the orientation vector and its parameter can now be modified
				
		_setP0			= false; //Set P0 --Left Click--
		_setP1			= false; //Set P1 --Left Click--
		_setP2			= false; //Set P2 --Left Click--
		_setTheta		= false; //activated with shift + LCM for rotation in 3D
}


BeesBookTagMatcher::~BeesBookTagMatcher(void)
{
}

void BeesBookTagMatcher::track		( ulong, cv::Mat& ){}
void BeesBookTagMatcher::paint		( cv::Mat& image )
{	
	if (_Grids.size() > 0)
	{
		drawSetTags(image);
	}
	if (_setTag)
	{
		drawOrientation(image, orient);
	}	
	else if(_activeTag)
	{		
		drawActiveTag(image);
	}		
}
void BeesBookTagMatcher::reset		(){}

//check if MOUSE BUTTON IS CLICKED
void BeesBookTagMatcher::mousePressEvent	( QMouseEvent * e )
{
	//check if LEFT button is clicked
	if ( e->button() == Qt::LeftButton)
	{		
		//check for SHIFT modifier
		if(Qt::ShiftModifier == QApplication::keyboardModifiers())
		{	
			if (_activeTag) // The Tag is active and can now be modified
			{
				_setTheta = true;
				if (selectPoint(cv::Point(e->x(), e->y())))
					emit update();
			}			
		}
		//check for CTRL modifier
		else if(Qt::ControlModifier == QApplication::keyboardModifiers())	//a new tag is generated
		{
			if (_ready)
				setTag(cv::Point(e->x(), e->y()));
		}
		//without modifier
		else
		{
			if (_activeTag) // The Tag is active and can now be modified
			{
				//if clicked in one of the bit cells, its value is changed
				if (dist(g.centerGrid, cv::Point(e->x(), e->y())) > 2 && dist(g.centerGrid, cv::Point(e->x(), e->y())) < g.axesGrid.height)
					g.updateID(cv::Point(e->x(), e->y()));
				else
					//if clicked on one of the set points, the point is activated
					if (selectPoint(cv::Point(e->x(), e->y())))
						emit update();
				//otherwise checks if one of the other tags is selected
					else
						selectTag(cv::Point(e->x(), e->y()));
			}
		}
	}
	//check if RIGHT button is clicked
	if ( e->button() == Qt::RightButton)
	{			
		//check for SHIFT modifier
		if(Qt::ShiftModifier == QApplication::keyboardModifiers())
		{
		}			
		//check for CTRL modifier
		if (Qt::ControlModifier == QApplication::keyboardModifiers())
		{
			cancelTag(); //The Tag being currently configured is cancelled
		}
	}	
	
}
//check if pointer MOVES
void BeesBookTagMatcher::mouseMoveEvent		( QMouseEvent * e )
{
	if (_setTag) //The tag is being set
	{
		orient[1] = cv::Point(e->x(), e->y());	
		emit update();
	}
	else if (_setP0) //The tag is being translated
	{	
		if (!_setTheta)
			g.translation(cv::Point(e->x(), e->y()));
		emit update();				
	}
	else if (_setP1) //The orientation of the tag is being modified
	{	
		if (!_setTheta) 
			g.orientation(cv::Point(e->x(), e->y()));
		else
			setTheta(cv::Point(e->x(), e->y()));
		emit update();
	}	
	else if (_setP2) //The space angle (theta) is being modified
	{	
		if (!_setTheta)
			emit update();
		else
			setTheta(cv::Point(e->x(), e->y()));
		emit update();
	}
}


//check if MOUSE BUTTON IS RELEASED
void BeesBookTagMatcher::mouseReleaseEvent	( QMouseEvent * e )
{
	// left button released
	if (e->button() == Qt::LeftButton)
	{
		if (_setTag)
		{
			_setTag = false;
			_activeTag = true;
			_ready = true;			
			g = myNewGrid(orient[0], atan2(orient[1].x - orient[0].x, orient[1].y - orient[0].y) - M_PI / 2);	//the tag's center is set.
		}		
		else if (_setP0)
			_setP0 = false;	
		else if (_setP1)
			_setP1 = false;
		else if (_setP2)
			_setP2 = false;
		if (_setTheta)
			_setTheta = false;
		emit update();
	}
	// right button released
	if (e->button() == Qt::RightButton)
	{		
	}
}
//check if WHEEL IS ACTIVE
void BeesBookTagMatcher::mouseWheelEvent	( QWheelEvent * e)
{
	if (_activeTag) // The Grid is active for draging
	{					
			g.scale = g.scale + e->delta() / 96*0.05;		//scale variable is updated by 0.05
			std::cout << "scale " << g.scale << std::endl;
			g.updateAxes();			
			emit update();		
	}	
}

//BeesBookTagMatcher private member functions

//DRAWING FUNCTIONS

//function that draws the set Tags so far.
void BeesBookTagMatcher::drawSetTags(cv::Mat image)
{
    for (size_t i = 0; i < _Grids.size(); i++)
	{
		gtemp = myNewGrid(_Grids[i].scale, _Grids[i].centerGrid, _Grids[i].alpha, _Grids[i].theta, _Grids[i].phi, _Grids[i].rho, _Grids[i].ID);
		gtemp.drawFullTag(image, 2); //the grids are drawn as set
	}	
}

//this one draws a basic grid onto the display image
void BeesBookTagMatcher::drawOrientation(cv::Mat image, std::vector<cv::Point> orient)
{
	//std::cout << "DRAW ORIENTAION" << std::endl;
	cv::line(image, orient[0], orient[1], cv::Scalar(0, 0, 255), 1);		//the orientation vector is printed in red
}

//this one draws a basic grid onto the display image
void BeesBookTagMatcher::drawActiveTag(cv::Mat image)
{		
	//std::cout<<"DRAW ACTIVE GRID"<<std::endl;	
	g.drawFullTag(image,1); //the grid is drawn as active	
	for (int i = 0; i < 3; i++)
	{
		cv::circle(image, g.absPoints[i], 1, cv::Scalar(0, 0, 255), 1); //the point is drawn in red					
	}
	//active point in blue
	if (_setP0)
		cv::circle(image, g.absPoints[0], 1, cv::Scalar(255, 0, 0), 1); //the center is drawn in white	
	else if (_setP1)
		cv::circle(image, g.absPoints[1], 1, cv::Scalar(255, 0, 0), 1); //the center is drawn in white	
	else if (_setP2)
		cv::circle(image, g.absPoints[2], 1, cv::Scalar(255, 0, 0), 1); //the center is drawn in white
}

//this one cancels the active tag and activates the previous one.
void BeesBookTagMatcher::cancelTag()
{
	if (_Grids.size() > 0)
	{
		g = myNewGrid(_Grids[_Grids.size() - 1].scale, _Grids[_Grids.size() - 1].centerGrid, _Grids[_Grids.size() - 1].alpha, _Grids[_Grids.size() - 1].theta, _Grids[_Grids.size() - 1].phi, _Grids[_Grids.size() - 1].rho, _Grids[_Grids.size() - 1].ID); //previous Tag is loaded
		_Grids.pop_back(); //last tag is set as active and removed from the vector
	}
	emit update();
	return;
}

//this one is called while setting the tag (it initializes orient vector)
void BeesBookTagMatcher::setTag(cv::Point location)
{
	//If there is an active Tag, this is pushed into the _Grids vector and a new Tag is generated
	if (_activeTag)
		_Grids.push_back(g);

	_ready = false;
	_setTag = true;

	orient.clear();
	orient.push_back(location);
	orient.push_back(location);
	emit update();	//the orientation vector is drawn.
	return;
}

//this one checks if one of the set Points is selected, returns true when one of the points is selected
bool BeesBookTagMatcher::selectPoint(cv::Point location)
{
	bool answer = false;
	for (int i = 0; i<3; i++)
	{
		if (dist(location, g.absPoints[i])<2) //check if the pointer is on one of the points
		{
			switch (i)
			{
			case 0:
				_setP0 = true;
				answer = true;
				break;
			case 1:
				_setP1 = true;
				answer = true;	
				break;
			case 2:
				_setP2 = true;
				answer = true;		
				break;
			default:
				answer = false;
				break;
			}
			break;
		}
	}
	return answer;
}

//this one allows P1 and P2 to be modified to calculate the tag's angle in space.
void BeesBookTagMatcher::setTheta(cv::Point location)
{
	cv::Point3d			v1;
	cv::Point3d			v2;
	cv::Point3d			norm;

	//calculate the Z component for P1 and P2 in order to calculate a normal vector to them through cross product
	if (_setP1)
	{
		v1.x = g.relPoints[1].x;
		v1.y = g.relPoints[1].y;
		v1.z = sqrt(pow(g.scale*axisTag, 2) - (pow(v1.x, 2) + pow(v1.y, 2)));
		std::cout << "radius Tag " << g.scale*axisTag << std::endl;
		std::cout << "vector1 " << v1.x << " " << v1.y << " " << v1.z << std::endl;
	}
	else if (_setP2)
	{
		v2.x = g.relPoints[2].x;
		v2.y = g.relPoints[2].y;
		v2.z = sqrt(pow(g.scale*axisTag, 2) - (pow(v2.x, 2) + pow(v2.y, 2)));
		std::cout << "vector2 " << v2.x << " " << v2.y << " " << v2.z << std::endl;
	}
}

//this one checks if one of the already set Tags is selected
void BeesBookTagMatcher::selectTag(cv::Point location)
{
	if (_Grids.size()>0)
        for (size_t i = 0; i < _Grids.size(); i++)
		{
		if (dist(location, _Grids[i].centerTag) < _Grids[i].axesTag.height)
			{
				_Grids.push_back(g); //active tag is pushed back in the vector of Grids
				g = myNewGrid(_Grids[i].scale, _Grids[i].centerGrid, _Grids[i].alpha, _Grids[i].theta, _Grids[i].phi, _Grids[i].rho, _Grids[i].ID); //selected Tag is loaded
				_Grids.erase(_Grids.begin()+i); //The selected tag is erased from the vector
				emit update();
			}			
		}	
	return;
}

//this one calculates the distance between two points
double BeesBookTagMatcher::dist(cv::Point p1, cv::Point p2)
{	
	diff = p1-p2;
	return sqrt(diff.x*diff.x + diff.y*diff.y);	
}
