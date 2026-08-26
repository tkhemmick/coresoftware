#include <GenFit/ConstField.h>
#include <GenFit/FieldManager.h>
#include <GenFit/MaterialEffects.h>
#include <GenFit/TGeoMaterialInterface.h>

#include <TGeoManager.h>
#include <TGeoMaterial.h>
#include <TGeoMedium.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>

#include <GenFit/RKTrackRep.h>
#include <GenFit/Track.h>

#include <TMatrixDSym.h>
#include <TVector3.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>

#include <GenFit/FitStatus.h>
#include <GenFit/KalmanFitter.h>
#include <GenFit/SpacepointMeasurement.h>

#include <TVectorD.h>

#include <cmath>

struct BenchmarkConfig
{
  // Geometry, cm
  double innerRadius = 20.0;
  double outerRadius = 80.0;
  double halfLength = 100.0;
  double worldHalfSize = 200.0;
  double boundaryMargin = 1.0;

  // Field: GENFIT ConstField uses kilogauss.
  double fieldX = 0.0;
  double fieldY = 0.0;
  double fieldZ = 14.0;

  // Truth track
  int pdgCode = 13;
  double particleCharge = -1.0;
  TVector3 truthPosition = TVector3(21.0, 0.0, 0.0);
  TVector3 truthMomentum = TVector3(1.5, 0.3, 0.4);

  // Measurements
  unsigned int numberOfHits = 48;
  double sigmaXY = 0.02;
  double sigmaZ = 0.05;
};

int BuildGeometry(TGeoManager &geo, BenchmarkConfig& config);
void FillTheTracks(std::vector<genfit::Track *>& tracks, long unsigned int Ntrack, BenchmarkConfig& config);
void AddMeasurements(genfit::Track* track, BenchmarkConfig& config);
void FitTheTracks( std::vector<genfit::Track*>& tracks );

int main(int argc, char* argv[]) {
    // 1. Check if the user provided the argument
    if (argc < 2) {
        std::cerr << "Error: Missing integer argument.\n";
        std::cerr << "Usage: " << argv[0] << " <number>\n";
        return 1; 
    }

    long unsigned int Ntrack;
    try {
        // 2. Convert the C-style string argument to an integer
        Ntrack = std::stoul(argv[1]);
	std::cout << "We shall speed test over a total of " << Ntrack << " tracks." << std::endl;
    } 
    catch (const std::invalid_argument& e) {
        std::cerr << "Error: '" << argv[1] << "' is not a valid integer.\n";
        return 1;
    } 
    catch (const std::out_of_range& e) {
        std::cerr << "Error: '" << argv[1] << "' is too large for an int.\n";
        return 1;
    }

  
  //  Build the detector geometry...
  std::cout << "Building geometry..." << std::endl;
  BenchmarkConfig config;
  TGeoManager geometry(
      "FastGenFitGeometry",
      "Simple Argon TPC benchmark geometry");
  if (BuildGeometry(geometry, config) != 0)
    {
      return 1;
    }
  
  //  Assign a magnetic field (electric field is irrelevant...  
  std::cout << "Initializing magnetic field..." << std::endl;
  genfit::FieldManager::getInstance()->init(
      new genfit::ConstField(
          config.fieldX,
          config.fieldY,
          config.fieldZ));
  std::cout << "Magnetic field initialized." << std::endl;

  //  I don't know whatthis means...but here it is...
  std::cout << "Initializing GENFIT material interface..." << std::endl;
  genfit::MaterialEffects::getInstance()->init(
      new genfit::TGeoMaterialInterface());
  std::cout << "GENFIT material interface initialized." << std::endl;

  std::cout << "Creating GENFIT tracks..." << std::endl;
  
  std::vector<genfit::Track*> tracks;
  tracks.reserve(Ntrack);
  FillTheTracks(tracks, Ntrack, config);
  FitTheTracks(tracks);
  
  std::cout << "FastGenFit test sequence completed successfully."
            << std::endl;

  // CLean up then die.
  for (genfit::Track* track : tracks)
    {
      delete track;
    }
  return 0;
}



int BuildGeometry(TGeoManager &geometry, BenchmarkConfig &config)
  {

  TGeoMaterial* vacuum =
      new TGeoMaterial(
          "Vacuum",
          0.0,
          0.0,
          0.0);

  TGeoMedium* vacuumMedium =
      new TGeoMedium(
          "VacuumMedium",
          1,
          vacuum);

  TGeoMaterial* argon =
      new TGeoMaterial(
          "Argon",
          39.948,
          18.0,
          0.001662);

  TGeoMedium* argonMedium =
      new TGeoMedium(
          "ArgonMedium",
          2,
          argon);

  TGeoVolume* world =
      geometry.MakeBox(
          "World",
          vacuumMedium,
          config.worldHalfSize,
          config.worldHalfSize,
          config.worldHalfSize);

  geometry.SetTopVolume(world);

  TGeoVolume* tpcGas =
      geometry.MakeTube(
          "TPCGas",
          argonMedium,
          config.innerRadius,
          config.outerRadius,
          config.halfLength);

  world->AddNode(
      tpcGas,
      1);

  geometry.CloseGeometry();

  std::cout << "Geometry closed successfully." << std::endl;

  TGeoNode* innerNode =
      geometry.FindNode(
          10.0,
          0.0,
          0.0);

  TGeoNode* gasNode =
      geometry.FindNode(
          40.0,
          0.0,
          0.0);

  TGeoNode* outerNode =
      geometry.FindNode(
          config.halfLength,
          0.0,
          0.0);

  if (!innerNode || !gasNode || !outerNode)
  {
    std::cerr
        << "ERROR: geometry lookup returned a null node."
        << std::endl;
    return 1;
  }

  std::cout
      << "Material at r = 10 cm: "
      << innerNode->GetVolume()
             ->GetMedium()
             ->GetMaterial()
             ->GetName()
      << std::endl;

  std::cout
      << "Material at r = 40 cm: "
      << gasNode->GetVolume()
             ->GetMedium()
             ->GetMaterial()
             ->GetName()
      << std::endl;

  std::cout
      << "Material at r = 100 cm: "
      << outerNode->GetVolume()
             ->GetMedium()
             ->GetMaterial()
             ->GetName()
      << std::endl;

  return 0;
}


void FillTheTracks(std::vector<genfit::Track *>& tracks, long unsigned int Ntrack, BenchmarkConfig& config)
{
  
  while (tracks.size() < Ntrack)
    {

      // PDG code 13: negatively charged muon.
      genfit::RKTrackRep* trackRep =
	new genfit::RKTrackRep(config.pdgCode);
  
      
      // GENFIT Track expects one six-component seed vector:
      // x, y, z, px, py, pz.
      TVectorD stateSeed(6);
      
      stateSeed[0] = config.truthPosition.X();
      stateSeed[1] = config.truthPosition.Y();
      stateSeed[2] = config.truthPosition.Z();

      stateSeed[3] = config.truthMomentum.X();
      stateSeed[4] = config.truthMomentum.Y();
      stateSeed[5] = config.truthMomentum.Z();
      
      TMatrixDSym seedCovariance(6);
      seedCovariance.Zero();
      
      // Position variances in cm^2.
      seedCovariance(0, 0) = 0.1 * 0.1;
      seedCovariance(1, 1) = 0.1 * 0.1;
      seedCovariance(2, 2) = 0.1 * 0.1;
      
      // Momentum variances in (GeV/c)^2.
      seedCovariance(3, 3) = 0.1 * 0.1;
      seedCovariance(4, 4) = 0.1 * 0.1;
      seedCovariance(5, 5) = 0.1 * 0.1;
      
      genfit::Track* track =
	new genfit::Track(
			  trackRep,
			  stateSeed,
			  seedCovariance);
      
      AddMeasurements(track, config);
      track->checkConsistency();
      
      tracks.push_back(track);
    }
}
 
void AddMeasurements(genfit::Track* track, BenchmarkConfig &config)
{
  if (!track)
  {
    throw std::invalid_argument(
        "AddMeasurements received a null track pointer.");
  }

  // Keep measurements away from the exact material boundaries.
  double finalRadius = config.outerRadius - config.boundaryMargin;  // cm

  const double transverseMomentum =
      std::sqrt(
          config.truthMomentum.X() * config.truthMomentum.X() +
          config.truthMomentum.Y() * config.truthMomentum.Y());

  const double initialDirection =
      std::atan2(
          config.truthMomentum.Y(),
          config.truthMomentum.X());

  /*
   * For momentum in GeV/c, B in tesla and distance in cm:
   *
   *   |curvature| = 0.00299792458 |q| B / pT
   *
   * The minus sign gives the correct rotation direction from q(v x B).
   */

  const double magneticFieldTesla =
    0.1 * config.fieldZ;

  const double curvature =
    -config.particleCharge *
    0.00299792458 *
    magneticFieldTesla /
    transverseMomentum;

  // Return the ideal helix position after a transverse path length s.
  const auto HelixPosition =
      [&](const double transversePathLength)
      {
        const double direction =
            initialDirection +
            curvature * transversePathLength;

        const double x =
            config.truthPosition.X() +
            (std::sin(direction) -
             std::sin(initialDirection)) /
                curvature;

        const double y =
            config.truthPosition.Y() -
            (std::cos(direction) -
             std::cos(initialDirection)) /
                curvature;

        const double z =
            config.truthPosition.Z() +
            transversePathLength *
                config.truthMomentum.Z() /
                transverseMomentum;

        return TVector3(x, y, z);
      };

  /*
   * Find the path length at which the trajectory reaches r = 79 cm.
   * First expand the upper bound until it encloses the target radius.
   */
  double lowerPathLength = 0.0;
  double upperPathLength = config.halfLength;

  while (HelixPosition(upperPathLength).Perp() < finalRadius)
  {
    upperPathLength *= 2.0;

    if (upperPathLength > 10000.0)
    {
      throw std::runtime_error(
          "Generated helix never reached the requested outer radius.");
    }
  }

  // Refine the endpoint by bisection.
  for (int iteration = 0; iteration < 100; ++iteration)
  {
    const double middlePathLength =
        0.5 * (lowerPathLength + upperPathLength);

    if (HelixPosition(middlePathLength).Perp() < finalRadius)
    {
      lowerPathLength = middlePathLength;
    }
    else
    {
      upperPathLength = middlePathLength;
    }
  }

  const double finalPathLength =
      0.5 * (lowerPathLength + upperPathLength);

  // Add 48 ideal, ordered space points.
  for (unsigned int hitId = 0; hitId < config.numberOfHits; ++hitId)
  {
    const double fraction =
        static_cast<double>(hitId) /
        static_cast<double>(config.numberOfHits - 1);

    const double pathLength =
        fraction * finalPathLength;

    const TVector3 hitPosition =
        HelixPosition(pathLength);

    TVectorD coordinates(3);
    coordinates[0] = hitPosition.X();
    coordinates[1] = hitPosition.Y();
    coordinates[2] = hitPosition.Z();

    TMatrixDSym covariance(3);
    covariance.Zero();

    covariance(0, 0) = config.sigmaXY * config.sigmaXY;
    covariance(1, 1) = config.sigmaXY * config.sigmaXY;
    covariance(2, 2) = config.sigmaZ * config.sigmaZ;

    genfit::SpacepointMeasurement* measurement =
        new genfit::SpacepointMeasurement(
            coordinates,
            covariance,
            0,       // detector ID
            hitId,
            nullptr);

    track->insertMeasurement(measurement);
  }
}

void FitTheTracks(std::vector<genfit::Track*>& tracks)
{
  if (tracks.empty())
  {
    std::cerr << "ERROR: No tracks available for fitting." << std::endl;
    return;
  }

  std::cout << "Beginning GENFIT Kalman fitting of "
            << tracks.size()
            << " tracks..."
            << std::endl;

  // Construct the fitter before starting the clock.
  // We want to measure track-fitting time, not fitter construction.
  genfit::KalmanFitter fitter;

  unsigned long numberFit = 0;
  unsigned long numberConverged = 0;

  const auto startTime =
      std::chrono::steady_clock::now();

  for (genfit::Track* track : tracks)
  {
    fitter.processTrack(track);
    ++numberFit;

    genfit::FitStatus* fitStatus =
        track->getFitStatus();

    if (fitStatus && fitStatus->isFitConverged())
    {
      ++numberConverged;
    }
  }

  const auto stopTime =
      std::chrono::steady_clock::now();

  const std::chrono::duration<double> elapsed =
      stopTime - startTime;

  const double totalSeconds =
      elapsed.count();

  const double secondsPerTrack =
      totalSeconds /
      static_cast<double>(numberFit);

  const double tracksPerSecond =
      static_cast<double>(numberFit) /
      totalSeconds;

  std::cout << std::endl;
  std::cout << "========== GENFIT TIMING ==========" << std::endl;
  std::cout << "Tracks fitted:       "
            << numberFit << std::endl;
  std::cout << "Tracks converged:    "
            << numberConverged << std::endl;
  std::cout << "Total fitting time:  "
            << totalSeconds
            << " s" << std::endl;
  std::cout << "Time per track:      "
            << 1000.0 * secondsPerTrack
            << " ms" << std::endl;
  std::cout << "Fitting rate:        "
            << tracksPerSecond
            << " tracks/s" << std::endl;
  std::cout << "===================================" << std::endl;
}
