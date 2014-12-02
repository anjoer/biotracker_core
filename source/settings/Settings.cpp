#include "Settings.h"
#include "Messages.h"

#include <QFile>
#include <QMessageBox>

#pragma warning(push, 0)   
#pragma warning(disable:4503)

Settings::Settings()
{
	if (!QFile::exists(QString::fromStdString(CONFIGPARAM::CONFIGURATION_FILE))) {
		QMessageBox::warning(nullptr, "No configuration file",
		                     QString::fromStdString(MSGS::SYSTEM::MISSING_CONFIGURATION_FILE));
		_ptree = getDefaultParams();
		boost::property_tree::write_ini(CONFIGPARAM::CONFIGURATION_FILE, _ptree);
	} else {
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(CONFIGPARAM::CONFIGURATION_FILE, pt);
		_ptree = pt;
	}
}

const boost::property_tree::ptree Settings::getDefaultParams()
{
	boost::property_tree::ptree pt;

	pt.put(TRACKERPARAM::TRACKING_ENABLED, false);
	pt.put(CAPTUREPARAM::CAP_VIDEO_FILE, "");
	pt.put(PICTUREPARAM::PICTURE_FILE, "");
	pt.put(GUIPARAM::IS_SOURCE_VIDEO, true);

	return pt;
}

#pragma warning(pop)
