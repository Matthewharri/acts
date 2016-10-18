// This file is part of the ACTS project.
//
// Copyright (C) 2016 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// ExtrapolationEngine.h, ACTS project
///////////////////////////////////////////////////////////////////

#ifndef ACTS_EXTRAPOLATION_EXTRAPOLATIONENGINE_H
#define ACTS_EXTRAPOLATION_EXTRAPOLATIONENGINE_H 1

#include "ACTS/EventData/NeutralParameters.hpp"
#include "ACTS/EventData/TrackParameters.hpp"
#include "ACTS/Extrapolation/ExtrapolationCell.hpp"
#include "ACTS/Extrapolation/IExtrapolationEngine.hpp"
#include "ACTS/Extrapolation/INavigationEngine.hpp"
#include "ACTS/Extrapolation/IPropagationEngine.hpp"
#include "ACTS/Extrapolation/detail/ExtrapolationMacros.hpp"
#include "ACTS/Utilities/Logger.hpp"

namespace Acts {

class TrackingGeometry;
class Surface;
class BoundaryCheck;

/// @class ExtrapolationEngine
///
/// Master extrapolation engine for extrapolation through the TrackingGeometry.
///
/// It delegates the extrapolation to optimised engines, handing over the
/// ExtrapolationCell
/// as internal cache.
///
/// There are identical interfaces for charged and neutral track parameters.
///
/// Providing a destination surface is optional, if no destination surface is
/// given the extrapolation
/// process can be stopped by other directives, e.g. stopping at a certain path
/// limit, material limit
/// or with a change of detector signature.
///
class ExtrapolationEngine : virtual public IExtrapolationEngine
{
public:
  /// @struct Config
  ///
  /// Configuration struct to be used for this ExtrapolationEngine.
  /// This has to be prepared by the framework/main and can be either given to
  /// the ExtrapolationEngine at construction or
  /// with the setConfig method.
  ///
  struct Config
  {
    /// tracking geometry
    std::shared_ptr<const TrackingGeometry> trackingGeometry = nullptr;
    /// list of extrapolation engines
    std::vector<std::shared_ptr<IExtrapolationEngine>> extrapolationEngines{};
    /// helper propagator for navigation initialization
    std::shared_ptr<IPropagationEngine> propagationEngine = nullptr;
    /// navigation engine
    std::shared_ptr<INavigationEngine> navigationEngine = nullptr;
    /// output prefix
    std::string prefix = "[ME] - ";
    /// output postfix
    std::string postfix = " - ";
  };

  /// Constructor
  ///
  /// @param eeConfig is the configuration struct for this engine
  /// @param logger logging instance
  ExtrapolationEngine(const Config&           eeConfig,
                      std::unique_ptr<Logger> logger
                      = getDefaultLogger("ExtrapolationEngine", Logging::INFO));

  /// Destructor
  ~ExtrapolationEngine();

  using IExtrapolationEngine::extrapolate;
  /// Charged extrapolation - public interface
  ///
  /// @param ecCharged is the charged extrapolation cell that holds the cache
  /// @param sf is the (optional) destinaton surface
  /// @param bcheck is the boudnary check directive @todo shift to cell after
  /// splitting
  ///
  /// @return extrapolation code to indicate outcome
  ExtrapolationCode
  extrapolate(ExCellCharged&       ecCharged,
              const Surface*       sf   = nullptr,
              const BoundaryCheck& bcheck = true) const final;

  /// Neutral extrapolation - public interface
  ///
  /// @param ecNeutral is the neutral extrapolation cell that holds the cache
  /// @param sf is the (optional) destinaton surface
  /// @param bcheck is the boudnary check directive @todo shift to cell after
  /// splitting
  ///
  /// @return extrapolation code to indicate outcome
  ExtrapolationCode
  extrapolate(ExCellNeutral&       ecNeutral,
              const Surface*       sf   = nullptr,
              const BoundaryCheck& bcheck = true) const final;

  /// define for which geometry type this extrapolator is valid
  ///  - this is GLOBAL
  /// @return the Geometry type for navigation
  GeometryType
  geometryType() const final;

  /// Set configuration method
  ///
  /// @param eeConfig is the configuration struct
  void
  setConfiguration(const Config& eeConfig);

  /// Get configuration method
  Config
  getConfiguration() const;

  /// Set logging instance
  ///
  /// @param logger is the logging instance
  void
  setLogger(std::unique_ptr<Logger> logger);

protected:
  /// ExtrapolationEngine config object
  Config m_cfg;

private:
  /// Private access to the logging instance
  const Logger&
  logger() const
  {
    return *m_logger;
  }

  /// logger instance
  std::unique_ptr<Logger> m_logger;

  /// Main extrapolation method, templated to chared/neutral
  ///
  /// @param eCell ist he extrapolaiton cell
  /// @param sf is the (optional) destinaton surface
  /// @param dir is the additional direction prescription
  /// @param bcheck is the boudnary check directive @todo shift to cell after
  /// splitting
  ///
  /// @return extrapolation code to indicate outcome
  template <class T>
  ExtrapolationCode
  extrapolateT(ExtrapolationCell<T>& eCell,
               const Surface*        sf     = nullptr,
               PropDirection         dir    = alongMomentum,
               const BoundaryCheck&  bcheck = true) const;

  /// Main extrapolation method, templated to chared/neutral
  ///
  /// @param eCell ist he extrapolaiton cell
  /// @param sf is the (optional) destinaton surface
  /// @param dir is the additional direction prescription
  ///
  /// @return extrapolation code to indicate outcome
  template <class T>
  ExtrapolationCode
  initNavigation(ExtrapolationCell<T>& eCell,
                 const Surface*        sf  = nullptr,
                 PropDirection         dir = alongMomentum) const;
};

/// Return the geometry type, it's the master
inline GeometryType
ExtrapolationEngine::geometryType() const
{
  return Acts::Master;
}

/// Return the configuration object
inline ExtrapolationEngine::Config
ExtrapolationEngine::getConfiguration() const
{
  return m_cfg;
}

}  // end of namespace

#include "ACTS/Extrapolation/detail/ExtrapolationEngine.ipp"

#endif  // ACTS_EXTRAPOLATION_EXTRAPOLATIONENGINE_H
