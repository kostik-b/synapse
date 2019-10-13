#include <ConfigManager.h>

#define BOOST_TEST_MODULE MyConfigManagerTest

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE( ConfigManagerTest )
{
    TcpProbe::ConfigManager configManager;

    // int
    BOOST_CHECK (configManager.insertParam<int>("weight", 80) == true);

    try
    {
      double weightDouble = 0.0f;
      BOOST_CHECK (configManager.getParam<double>("weight", weightDouble) == false);
    }
    catch (std::invalid_argument& e)
    {
      BOOST_CHECK (strcmp(e.what(), "Parameter stored is of a different type") == 0);
    }

    int weightInt = 0;
    BOOST_REQUIRE (configManager.getParam<int>("weight", weightInt) == true);

    BOOST_CHECK (weightInt == 80);


    // double
    BOOST_CHECK (configManager.insertParam<double>("height", 176.0f) == true);

    double height = 0.0f;
    BOOST_CHECK (configManager.getParam<double>("height", height) == true);

    BOOST_CHECK (height == 176.0f);
    

    // string
    BOOST_CHECK (configManager.insertParam<std::string>("name", "konstantin") == true);

    std::string name;
    BOOST_CHECK (configManager.getParam<std::string>("name", name) == true);

    BOOST_CHECK (name == "konstantin");

    // bool
    BOOST_CHECK (configManager.insertParam<bool>("christian", true) == true);

    bool christian = false;
    BOOST_CHECK (configManager.getParam<bool>("christian", christian) == true);

    BOOST_CHECK (christian == true);
}
