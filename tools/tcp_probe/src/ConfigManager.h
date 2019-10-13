// Copyright Queen's University of Belfast 2012

#ifndef TCPPROBE_CONFIGMANAGER_H
#define TCPPROBE_CONFIGMANAGER_H

#include <map>
#include <string>
#include <stdexcept>

#include "ConfigParam.h"

namespace TcpProbe
{

class ParamTypeConcept
{
};

class ConfigManager
{
public:

  typedef std::map<std::string, TcpProbe::ParamAbstract*> ConfigMap;

  ConfigManager () {}
  
  template <typename ParamType>
  bool insertParam(const std::string& key, const ParamType& value)
  {
    // TODO: check param type with BOOST concept (this will be compile time checking)
    // create ParamConcrete
    ParamConcrete<ParamType>*		  param	    = new ParamConcrete<ParamType>(value);
    // try insert it into the map
    std::pair<ConfigMap::iterator, bool>  retValue  = mMap.insert (ConfigMap::value_type(key, param));

    // return if it failed or not
    return retValue.second;
  }

  template <typename ParamType>
  bool getParam (const std::string& key, ParamType& value)
  {
    // TODO: check param type with BOOST concept (this will be compile time checking)
    // find param, if not found return false
    ConfigMap::iterator iter = mMap.find (key);

    if (iter == mMap.end())
    {
      return false; // couldn't find such param
    }

    // if found, try to dynamic cast it to concrete type - if it fails, throw exception
    ParamConcrete<ParamType>* param = dynamic_cast<ParamConcrete<ParamType>* >(iter->second);

    if (!param)
    {
      throw std::invalid_argument ("Parameter stored is of a different type");
    }

    // otherwise set the param and return true
    value = param->getParam ();

    return true;
  }

private:
  ConfigMap mMap;

}; // class ConfigManager

} // namespace TcpProbe

#endif
