// Copyright Queen's University of Belfast, 2012

#ifndef TCPPROBE_CONFIGPARAM_H
#define TCPPROBE_CONFIGPARAM_H

namespace TcpProbe
{

class ParamAbstract
{
public:
  // just to make it more visually appealing
  ParamAbstract () {}
  virtual ~ParamAbstract () {} // virtual destructor is needed to make this class polymorphic
};

template<typename ParamType>
class ParamConcrete : public ParamAbstract
{
  public:
    ParamConcrete (const ParamType& param)
      : mParam (param)
    {}

    ParamType getParam ()
    {
      return mParam;
    }

  private:
    const ParamType mParam;

}; // class ParamConcrete

} // namespace TcpProbe

#endif
