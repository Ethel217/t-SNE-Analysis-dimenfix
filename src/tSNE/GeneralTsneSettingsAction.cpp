#include "GeneralTsneSettingsAction.h"
#include "TsneSettingsAction.h"

using namespace mv::gui;

GeneralTsneSettingsAction::GeneralTsneSettingsAction(TsneSettingsAction& tsneSettingsAction) :
    GroupAction(&tsneSettingsAction, "TSNE", true),
    _tsneSettingsAction(tsneSettingsAction),
    _knnAlgorithmAction(this, "kNN Algorithm"),
    _distanceMetricAction(this, "Distance metric"),
    _perplexityAction(this, "Perplexity"),
    _computationAction(this),
    _reinitAction(this, "Reintialize instead of recompute", false),
    _saveProbDistAction(this, "Save analysis to projects", false),

    _dimenFixAction(this, "Enable DimenFix", true),
    _modeAction(this, "Pushing mode"),
    _itersAction(this, "Push between iters"),
    _fixSelectionAction(this, "Fixed Axis"),
    _labelInputAction(this, "Input labels"),
    _rangeLimitInputAction(this, "Input range limit"),
    _rangeLimitLAction(this, "Lower range limit"),
    _rangeLimitUAction(this, "Upper range limit"),
    _classOrderAction(this, "Class ordering"),
    _switchAxisAction(this, "Switch to new axis"),
    _alphaAction(this, "Overlap control")
{
    addAction(&_knnAlgorithmAction);
    addAction(&_distanceMetricAction);
    addAction(&_perplexityAction);
    addAction(&_itersAction);
    addAction(&_dimenFixAction);
    addAction(&_switchAxisAction);
    addAction(&_modeAction);
    addAction(&_fixSelectionAction);
    addAction(&_classOrderAction);
    addAction(&_alphaAction);

    addAction(&_labelInputAction);
    addAction(&_rangeLimitInputAction);
    addAction(&_rangeLimitLAction);
    addAction(&_rangeLimitUAction);
    
    _computationAction.addActions();

    addAction(&_reinitAction);
    addAction(&_saveProbDistAction);
    
    

    _knnAlgorithmAction.setDefaultWidgetFlags(OptionAction::ComboBox);
    _modeAction.setDefaultWidgetFlags(OptionAction::ComboBox);
    _fixSelectionAction.setDefaultWidgetFlags(OptionAction::ComboBox);
    _distanceMetricAction.setDefaultWidgetFlags(OptionAction::ComboBox);
    _perplexityAction.setDefaultWidgetFlags(IntegralAction::SpinBox | IntegralAction::Slider);
    _itersAction.setDefaultWidgetFlags(IntegralAction::SpinBox | IntegralAction::Slider);
    _classOrderAction.setDefaultWidgetFlags(OptionAction::ComboBox);
    _alphaAction.setDefaultWidgetFlags(DecimalAction::SpinBox | DecimalAction::Slider);

    _knnAlgorithmAction.initialize(QStringList({ "FLANN", "HNSW", "ANNOY" }), "FLANN");
    _modeAction.initialize(QStringList({ "CLIPPING", "GAUSSIAN", "RESCALE" }), "CLIPPING");
    _fixSelectionAction.initialize(QStringList({ "class_label", "feature_value", "input" }), "class_label");
    _classOrderAction.initialize(QStringList({ "random", "avg", "disable" }), "random");
    _distanceMetricAction.initialize(QStringList({ "Euclidean", "Cosine", "Inner Product", "Manhattan", "Hamming", "Dot" }), "Euclidean");
    _perplexityAction.initialize(2, 50, 30);
    _itersAction.initialize(1, 50, 2);
    _alphaAction.initialize(0.0f, 3.0f, 1.0f);

    _reinitAction.setToolTip("Instead of recomputing knn, simply re-initialize t-SNE embedding and recompute gradient descent.");
    _saveProbDistAction.setToolTip("When saving the t-SNE analysis with your project, you can compute additional iterations without recomputing similarities from scratch.");

    const auto updateKnnAlgorithm = [this]() -> void {
        if (_knnAlgorithmAction.getCurrentText() == "FLANN")
            _tsneSettingsAction.getKnnParameters().setKnnAlgorithm(hdi::dr::knn_library::KNN_FLANN);

        if (_knnAlgorithmAction.getCurrentText() == "HNSW")
            _tsneSettingsAction.getKnnParameters().setKnnAlgorithm(hdi::dr::knn_library::KNN_HNSW);

        if (_knnAlgorithmAction.getCurrentText() == "ANNOY")
            _tsneSettingsAction.getKnnParameters().setKnnAlgorithm(hdi::dr::knn_library::KNN_ANNOY);
    };

    const auto updateMode = [this]() -> void {
        if (_modeAction.getCurrentText() == "CLIPPING")
            _tsneSettingsAction.getTsneParameters().setMode("clipping");

        if (_modeAction.getCurrentText() == "GAUSSIAN")
            _tsneSettingsAction.getTsneParameters().setMode("gaussian");

        if (_modeAction.getCurrentText() == "RESCALE")
            _tsneSettingsAction.getTsneParameters().setMode("rescale");
    };

    const auto updateClassOrder = [this]() -> void {
        if (_classOrderAction.getCurrentText() == "random")
            _tsneSettingsAction.getTsneParameters().setClassOrder("random");

        if (_classOrderAction.getCurrentText() == "avg")
            _tsneSettingsAction.getTsneParameters().setClassOrder("avg");

        if (_classOrderAction.getCurrentText() == "disable")
            _tsneSettingsAction.getTsneParameters().setClassOrder("disable");
    };

    const auto updateFixSelection = [this]() -> void {
        if (_fixSelectionAction.getCurrentText() == "class_label")
            _tsneSettingsAction.getTsneParameters().setFixSelection("class_label");

        if (_fixSelectionAction.getCurrentText() == "feature_value")
            _tsneSettingsAction.getTsneParameters().setFixSelection("feature_value");

        if (_fixSelectionAction.getCurrentText() == "input")
            _tsneSettingsAction.getTsneParameters().setFixSelection("input");
    };

    const auto updateDistanceMetric = [this]() -> void {
        if (_distanceMetricAction.getCurrentText() == "Euclidean")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_EUCLIDEAN);

        if (_distanceMetricAction.getCurrentText() == "Cosine")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_COSINE);

        if (_distanceMetricAction.getCurrentText() == "Inner Product")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_INNER_PRODUCT);

        if (_distanceMetricAction.getCurrentText() == "Manhattan")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_MANHATTAN);

        if (_distanceMetricAction.getCurrentText() == "Hamming")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_HAMMING);

        if (_distanceMetricAction.getCurrentText() == "Dot")
            _tsneSettingsAction.getKnnParameters().setKnnDistanceMetric(hdi::dr::knn_distance_metric::KNN_METRIC_DOT);
    };

    const auto updateAlpha = [this]() -> void {
        _tsneSettingsAction.getTsneParameters().setAlpha(_alphaAction.getValue());
    };

    const auto updateNumIterations = [this]() -> void {
        _tsneSettingsAction.getTsneParameters().setNumIterations(_computationAction.getNumIterationsAction().getValue());
    };

    const auto updatePerplexity = [this]() -> void {
        _tsneSettingsAction.getTsneParameters().setPerplexity(_perplexityAction.getValue());
    };

    const auto updateIters = [this]() -> void {
        _tsneSettingsAction.getTsneParameters().setIters(_itersAction.getValue());
    };

    const auto updateCoreUpdate = [this]() -> void {
        _tsneSettingsAction.getTsneParameters().setUpdateCore(_computationAction.getUpdateIterationsAction().getValue());
    };

    // currently unused
    //const auto isResettable = [this]() -> bool {
    //    if (_knnAlgorithmAction.isResettable())
    //        return true;

    //    if (_distanceMetricAction.isResettable())
    //        return true;

    //    if (_computationAction.getNumIterationsAction().isResettable())
    //        return true;

    //    if (_computationAction.getUpdateIterationsAction().isResettable())
    //        return true;

    //    if (_perplexityAction.isResettable())
    //        return true;

    //    return false;
    //};

    const auto updateReadOnly = [this]() -> void {
        const auto enable = !isReadOnly();

        _knnAlgorithmAction.setEnabled(enable);
        _distanceMetricAction.setEnabled(enable);
        _computationAction.getNumIterationsAction().setEnabled(enable);
        _perplexityAction.setEnabled(enable);
        _computationAction.getUpdateIterationsAction().setEnabled(enable);
        _reinitAction.setEnabled(enable);
        _saveProbDistAction.setEnabled(enable);

        _dimenFixAction.setEnabled(enable);
        _modeAction.setEnabled(enable);
        _classOrderAction.setEnabled(enable);
        _itersAction.setEnabled(enable);
        _fixSelectionAction.setEnabled(enable);
        _alphaAction.setEnabled(enable);

        _rangeLimitInputAction.setEnabled(enable);
        _rangeLimitLAction.setEnabled(enable);
        _rangeLimitUAction.setEnabled(enable);
        _labelInputAction.setEnabled(enable);
    };

    connect(&_knnAlgorithmAction, &OptionAction::currentIndexChanged, this, [this, updateKnnAlgorithm](const std::int32_t& currentIndex) {
        updateKnnAlgorithm();
    });

    connect(&_modeAction, &OptionAction::currentIndexChanged, this, [this, updateMode](const std::int32_t& currentIndex) {
        updateMode();
    });

    connect(&_classOrderAction, &OptionAction::currentIndexChanged, this, [this, updateClassOrder](const std::int32_t& currentIndex) {
        updateClassOrder();
    });

    connect(&_fixSelectionAction, &OptionAction::currentIndexChanged, this, [this, updateFixSelection](const std::int32_t& currentIndex) {
        updateFixSelection();
    });

    connect(&_dimenFixAction, &ToggleAction::toggled, this, [this, updateCoreUpdate](const bool toggled) {
        _tsneSettingsAction.getTsneParameters().setDimenfix(toggled);
    });

    connect(&_switchAxisAction, &ToggleAction::toggled, this, [this, updateCoreUpdate](const bool toggled) {
        _tsneSettingsAction.getTsneParameters().setSwitchAxis(toggled);
    });

    _rangeLimitInputAction.setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        if (dataset->getDataType() == PointType)
        {
            const auto pointDataset = Dataset<Points>(dataset);
            if (pointDataset->getNumDimensions() >= 1) // TODO: didnt check size alignment
                return true;
            
            return false;
        }

        return false;

        });

    connect(&_rangeLimitInputAction, &DatasetPickerAction::datasetPicked , this, [this](mv::Dataset<mv::DatasetImpl> pickedDataset) {
        _rangeLimitLAction.setPointsDataset(pickedDataset);
        _rangeLimitUAction.setPointsDataset(pickedDataset);

        _rangeLimitLAction.setCurrentDimensionIndex(0);
        _rangeLimitUAction.setCurrentDimensionIndex(1);
    });

    _labelInputAction.setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        if (dataset->getDataType() == PointType)
        {
            const auto pointDataset = Dataset<Points>(dataset);
            if (pointDataset->getNumDimensions() == 1)
                return true;
            
            return false;
        }
        return false;
    });

    // connect(&_labelInputAction, &DatasetPickerAction::datasetPicked , this, [this](mv::Dataset<mv::DatasetImpl> pickedDataset) {
    //     _labelInputAction.setPointsDataset(pickedDataset);
    // });

    connect(&_distanceMetricAction, &OptionAction::currentIndexChanged, this, [this, updateDistanceMetric](const std::int32_t& currentIndex) {
        updateDistanceMetric();
    });

    connect(&_computationAction.getNumIterationsAction(), &IntegralAction::valueChanged, this, [this, updateNumIterations](const std::int32_t& value) {
        updateNumIterations();
    });

    connect(&_alphaAction, &DecimalAction::valueChanged, this, [this, updateAlpha](const float& value) {
        updateAlpha();
    });

    connect(&_perplexityAction, &IntegralAction::valueChanged, this, [this, updatePerplexity](const std::int32_t& value) {
        updatePerplexity();
    });

    connect(&_itersAction, &IntegralAction::valueChanged, this, [this, updateIters](const std::int32_t& value) {
        updateIters();
    });

    connect(&_computationAction.getUpdateIterationsAction(), &IntegralAction::valueChanged, this, [this, updateCoreUpdate](const std::int32_t& value) {
        updateCoreUpdate();
    });

    connect(&_reinitAction, &ToggleAction::toggled, this, [this, updateCoreUpdate](const bool toggled) {
        QString newText = (toggled) ? "Reinit" : "Start";
        _computationAction.getStartComputationAction().setText(newText);
        });

    connect(this, &GroupAction::readOnlyChanged, this, [this, updateReadOnly](const bool& readOnly) {
        updateReadOnly();
    });

    updateKnnAlgorithm();
    updateDistanceMetric();
    updateNumIterations();
    updatePerplexity();
    updateCoreUpdate();
    updateReadOnly();

    updateMode();
    updateClassOrder();
    updateIters();
    updateAlpha();

    _reinitAction.setEnabled(false);    // only enable after first compute
    _reinitAction.setCheckable(false);  // only enable after first compute
}

std::vector<float> GeneralTsneSettingsAction::getLabel(size_t numPoints)
{
    assert(numPoints > 0);
    std::vector<float> initLabels(numPoints);
    
    if (_labelInputAction.getCurrentDataset().isValid())
    {
        auto initData = _labelInputAction.getCurrentDataset<Points>();

        qDebug() << "Labels are loading... " << initData->getGuiName();

        initData->populateDataForDimensions(initLabels, std::vector<int32_t>{ 0 });
    }
    else
    {
        initLabels = std::vector<float>(numPoints, 0.0f);
    }

    return initLabels;
}

std::vector<float> GeneralTsneSettingsAction::getInitRanges(size_t numPoints)
{
    assert(numPoints > 0);
    std::vector<float> initRanges(numPoints * 2);
    
    auto initData = _rangeLimitInputAction.getCurrentDataset<Points>();
    auto xDim = _rangeLimitLAction.getCurrentDimensionIndex();
    auto yDim = _rangeLimitUAction.getCurrentDimensionIndex();
    
    qDebug() << "Ranges initializing... " << initData->getGuiName();

    initData->populateDataForDimensions(initRanges, std::vector<int32_t>{ xDim, yDim });

    return initRanges;
}

void GeneralTsneSettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _knnAlgorithmAction.fromParentVariantMap(variantMap);
    _distanceMetricAction.fromParentVariantMap(variantMap);
    _perplexityAction.fromParentVariantMap(variantMap);
    _computationAction.fromParentVariantMap(variantMap);
    _reinitAction.fromParentVariantMap(variantMap);
    _saveProbDistAction.fromParentVariantMap(variantMap);

    _dimenFixAction.fromParentVariantMap(variantMap);
    _switchAxisAction.fromParentVariantMap(variantMap);
    _modeAction.fromParentVariantMap(variantMap);
    _alphaAction.fromParentVariantMap(variantMap);
    _classOrderAction.fromParentVariantMap(variantMap);
    _itersAction.fromParentVariantMap(variantMap);
    _fixSelectionAction.fromParentVariantMap(variantMap);
    _rangeLimitInputAction.fromParentVariantMap(variantMap);
    _rangeLimitLAction.fromParentVariantMap(variantMap);
    _rangeLimitUAction.fromParentVariantMap(variantMap);
    _labelInputAction.fromParentVariantMap(variantMap); // TODO: labels might not be available
}

QVariantMap GeneralTsneSettingsAction::toVariantMap() const
{
    QVariantMap variantMap = GroupAction::toVariantMap();

    _knnAlgorithmAction.insertIntoVariantMap(variantMap);
    _distanceMetricAction.insertIntoVariantMap(variantMap);
    _perplexityAction.insertIntoVariantMap(variantMap);
    _computationAction.insertIntoVariantMap(variantMap);
    _reinitAction.insertIntoVariantMap(variantMap);
    _saveProbDistAction.insertIntoVariantMap(variantMap);

    _dimenFixAction.insertIntoVariantMap(variantMap);
    _switchAxisAction.insertIntoVariantMap(variantMap);
    _modeAction.insertIntoVariantMap(variantMap);
    _alphaAction.insertIntoVariantMap(variantMap);
    _classOrderAction.insertIntoVariantMap(variantMap);
    _itersAction.insertIntoVariantMap(variantMap);
    _fixSelectionAction.insertIntoVariantMap(variantMap);

    _rangeLimitInputAction.insertIntoVariantMap(variantMap);
    _rangeLimitLAction.insertIntoVariantMap(variantMap);
    _rangeLimitUAction.insertIntoVariantMap(variantMap);
    _labelInputAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
