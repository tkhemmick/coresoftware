//
// Inspired by code from ATLAS.  Thanks!
//

#include "flowAfterburner.h"

#include <HepMC/GenEvent.h>
#include <HepMC/GenVertex.h>
#include <HepMC/GenParticle.h>
#include <HepMC/GenRanges.h>
#include <HepMC/IO_AsciiParticles.h>
#include <HepMC/IO_GenEvent.h>

#include <CLHEP/Random/RandomEngine.h>
#include <CLHEP/Random/MTwistEngine.h>
#include <CLHEP/Random/RandFlat.h>
#include <CLHEP/Vector/LorentzVector.h>
#include <CLHEP/Geometry/Point3D.h>

#include <boost/version.hpp> // to get BOOST_VERSION
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 8 && BOOST_VERSION == 106000 )
#pragma GCC diagnostic ignored "-Wshadow"
#pragma message "ignoring bogus gcc warning in boost header ptree.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#pragma GCC diagnostic warning "-Wshadow"
#else
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#endif

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>


#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

CLHEP::HepRandomEngine * engine;

int
main ()
{
  using namespace boost::property_tree;
  iptree pt;

  std::ifstream config_file("flowAfterburner.xml");

  if (config_file)
    {
      // Read XML configuration file.
      read_xml (config_file, pt);
    }

  long randomSeed = pt.get ("FLOWAFTERBURNER.RANDOM.SEED", 11793);
  engine = new CLHEP::MTwistEngine (randomSeed);

  std::string input = pt.get("FLOWAFTERBURNER.INPUT", "sHijing.dat");
  std::string output = pt.get("FLOWAFTERBURNER.OUTPUT", "flowAfterburner.dat");

  float mineta = pt.get("FLOWAFTERBURNER.CUTS.MINETA", -1.0);
  float maxeta = pt.get("FLOWAFTERBURNER.CUTS.MAXETA", 1.0);

  float minpt = pt.get("FLOWAFTERBURNER.CUTS.MINPT", 0.0);
  float maxpt = pt.get("FLOWAFTERBURNER.CUTS.MAXPT", 100.0);

  std::string algorithmName = pt.get("FLOWAFTERBURNER.ALGORITHM", "JJNEW");

  // Open input file.
  HepMC::IO_GenEvent ascii_in (input.c_str(), std::ios::in);
  HepMC::IO_GenEvent ascii_out (output.c_str(), std::ios::out);
  HepMC::GenEvent *evt;

  while (ascii_in >> evt)
    {
      flowAfterburner(evt,engine,algorithmName,mineta,maxeta,minpt,maxpt);

      ascii_out << evt;
      delete evt;
    }
}
