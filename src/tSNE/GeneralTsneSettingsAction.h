#pragma once

#include "actions/IntegralAction.h"
#include "actions/OptionAction.h"
#include "actions/ToggleAction.h"
#include "actions/DatasetPickerAction.h"

#include "TsneComputationAction.h"

using namespace mv::gui;

class TsneSettingsAction;

/**
 * General TSNE setting action class
 *
 * Actions class for general TSNE settings
 *
 * @author Thomas Kroes
 */
class GeneralTsneSettingsAction : public GroupAction
{
public:

    /**
     * Constructor
     * @param tsneSettingsAction Reference to TSNE settings action
     */
    GeneralTsneSettingsAction(TsneSettingsAction& tsneSettingsAction);

    std::vector<float> getLabel(size_t numPoints); // TODO: num_points check should be here
public: // Action getters

    TsneSettingsAction& getTsneSettingsAction() { return _tsneSettingsAction; };
    OptionAction& getKnnAlgorithmAction() { return _knnAlgorithmAction; };
    OptionAction& getDistanceMetricAction() { return _distanceMetricAction; };
    IntegralAction& getNumIterationsAction() { return _computationAction.getNumIterationsAction(); };
    IntegralAction& getNumberOfComputatedIterationsAction() { return _computationAction.getNumberOfComputatedIterationsAction(); };
    IntegralAction& getPerplexityAction() { return _perplexityAction; };
    TsneComputationAction& getComputationAction() { return _computationAction; };
    ToggleAction& getReinitAction() { return _reinitAction; };
    ToggleAction& getSaveProbDistAction() { return _saveProbDistAction; };

    ToggleAction& getDimenFixAction() { return _dimenFixAction; };
    OptionAction& getModeAction() { return _modeAction; };
    IntegralAction& getItersAction() { return  _itersAction;};
    OptionAction& getFixSelectionAction() { return _fixSelectionAction; };
    OptionAction& getClassOrderAction() { return _classOrderAction; };

    DatasetPickerAction& getLabelInputAction() { return _labelInputAction; };
    DatasetPickerAction& getRangeLimitInputAction() { return _rangeLimitInputAction; };


public: // Serialization

    /**
     * Load plugin from variant map
     * @param Variant map representation of the plugin
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;

protected:
    TsneSettingsAction&     _tsneSettingsAction;                    /** Reference to parent tSNE settings action */
    OptionAction            _knnAlgorithmAction;                    /** KNN algorithm action */
    OptionAction            _distanceMetricAction;                  /** Distance metric action */
    IntegralAction          _perplexityAction;                      /** Perplexity action */
    TsneComputationAction   _computationAction;                     /** Computation action */
    ToggleAction            _reinitAction;                          /** Whether to re-initialize instead of recomputing from scratch */
    ToggleAction            _saveProbDistAction;                    /** Save t-SNE to projects action */

    ToggleAction            _dimenFixAction;     // true or false
    OptionAction            _modeAction;
    IntegralAction          _itersAction;
    OptionAction            _fixSelectionAction;
    OptionAction            _classOrderAction;

    DatasetPickerAction     _labelInputAction;
    DatasetPickerAction     _rangeLimitInputAction;
};
