#ifndef Calibration_EcalCalibAlgos_PhiSymmetryCalibration_h
#define Calibration_EcalCalibAlgos_PhiSymmetryCalibration_h

//
// Package:    Calibration/EcalCalibAlgos
// Class:      PhiSymmetryCalibration
//
//
// Description: performs phi-symmetry calibration
//
//
// Original Author:  David Futyan
//
// Responsible    :  Stefano Argiro, Valentina Sola
//

#include <vector>

#include "Calibration/EcalCalibAlgos/interface/EcalGeomPhiSymHelper.h"

// Framework
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ProducerBase.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"

#include "CondFormats/EcalObjects/interface/EcalIntercalibConstants.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsRcd.h"
#include "CondFormats/EcalObjects/interface/EcalChannelStatus.h"
#include "CondFormats/DataRecord/interface/EcalChannelStatusRcd.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"

class TH1F;

class PhiSymmetryCalibration
    : public edm::one::EDAnalyzer<edm::one::SharedResources, edm::one::WatchRuns, edm::one::WatchLuminosityBlocks> {
public:
  /// Constructor
  PhiSymmetryCalibration(const edm::ParameterSet& iConfig);

  /// Destructor
  ~PhiSymmetryCalibration() override;

  /// Called at beginning of job
  void beginJob() override;
  void beginRun(edm::Run const&, const edm::EventSetup&) override;
  void endRun(edm::Run const&, const edm::EventSetup&) override;
  void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
  void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

  /// Called at end of job
  void endJob() override;

  /// Called at each event
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  ///
  void setUp(const edm::EventSetup& setup);

private:
  // private member functions

  void getKfactors();

  // private data members

  static const int kNMiscalBinsEB = 21;
  static const float kMiscalRangeEB;

  static const int kNMiscalBinsEE = 41;
  static const float kMiscalRangeEE;

  EcalGeomPhiSymHelper e_;

  // Transverse energy sum arrays
  double etsum_barl_[kBarlRings][kBarlWedges][kSides];
  double etsum_endc_[kEndcWedgesX][kEndcWedgesX][kSides];
  double etsum_endc_uncorr[kEndcWedgesX][kEndcWedgesX][kSides];
  double etsumMean_barl_[kBarlRings];
  double etsumMean_endc_[kEndcEtaRings];

  unsigned int nhits_barl_[kBarlRings][kBarlWedges][kSides];
  unsigned int nhits_endc_[kEndcWedgesX][kEndcWedgesX][kSides];

  double etsum_barl_miscal_[kNMiscalBinsEB][kBarlRings];
  double etsum_endc_miscal_[kNMiscalBinsEE][kEndcEtaRings];

  double esumMean_barl_[kBarlRings];
  double esumMean_endc_[kEndcEtaRings];

  // factors to convert from ET sum deviation to miscalibration
  double k_barl_[kBarlRings];
  double k_endc_[kEndcEtaRings];
  double miscalEB_[kNMiscalBinsEB];
  double miscalEE_[kNMiscalBinsEE];

  std::vector<DetId> barrelCells;
  std::vector<DetId> endcapCells;

  // input calibration constants
  double oldCalibs_barl[kBarlRings][kBarlWedges][kSides];
  double oldCalibs_endc[kEndcWedgesX][kEndcWedgesY][kSides];

  // new calibration constants
  double newCalibs_barl[kBarlRings][kBarlWedges][kSides];
  double newCalibs_endc[kEndcWedgesX][kEndcWedgesX][kSides];

  // calibration constants not multiplied by old ones
  float epsilon_M_barl[kBarlRings][kBarlWedges][kSides];
  float epsilon_M_endc[kEndcWedgesX][kEndcWedgesY][kSides];

  // calibration const not corrected for k
  float rawconst_barl[kBarlRings][kBarlWedges][kSides];
  float rawconst_endc[kEndcWedgesX][kEndcWedgesX][kSides];

  // steering parameters

  const std::string ecalHitsProducer_;
  const std::string barrelHits_;
  const std::string endcapHits_;

  const edm::EDGetTokenT<EBRecHitCollection> ebRecHitToken_;
  const edm::EDGetTokenT<EERecHitCollection> eeRecHitToken_;
  const edm::ESGetToken<EcalChannelStatus, EcalChannelStatusRcd> channelStatusToken_;
  const edm::ESGetToken<CaloGeometry, CaloGeometryRecord> geometryToken_;

  // energy cut in the barrel
  const double eCut_barl_;

  // parametrized energy cut EE : e_cut = ap + eta_ring*b
  const double ap_;
  const double b_;

  const int eventSet_;
  /// threshold in channel status beyond which channel is marked bad
  const int statusThreshold_;

  static const int kMaxEndciPhi = 360;

  float phi_endc[kMaxEndciPhi][kEndcEtaRings];

  const bool reiteration_;
  const std::string oldcalibfile_;  //searched for in Calibration/EcalCalibAlgos/data

  /// the old calibration constants (when reiterating, the last ones derived)
  EcalIntercalibConstants oldCalibs_;

  bool isfirstpass_;

  // Et and E spectra
  std::vector<TH1F*> et_spectrum_b_histos;  //kBarlEtaRings
  std::vector<TH1F*> e_spectrum_b_histos;
  std::vector<TH1F*> et_spectrum_e_histos;  //kEndcEtaRings
  std::vector<TH1F*> e_spectrum_e_histos;

  bool spectra;
  int nevents_;
  int eventsinrun_;
  int eventsinlb_;
};

#endif
