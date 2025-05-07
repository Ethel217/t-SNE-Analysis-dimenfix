#include "TsneAnalysis.h"

#include "hdi/utils/glad/glad.h"
#include "OffscreenBuffer.h"

#include <cassert>
#include <vector>

#include <QCoreApplication>
#include <QDebug>
#include <fstream>
#include <set>
#include <limits>

#include "hdi/utils/scoped_timers.h"

using namespace mv;

TsneWorker::TsneWorker(TsneParameters tsneParameters) :
    _currentIteration(0),
    _tsneParameters(tsneParameters),
    _knnParameters(),
    _numPoints(0),
    _numDimensions(0),
    _data(),
    _probabilityDistribution(),
    _hasProbabilityDistribution(false),
    _GPGPU_tSNE(),
    _CPU_tSNE(),
    _embedding(),
    _outEmbedding(),
    _offscreenBuffer(nullptr),
    _shouldStop(false),
    _parentTask(nullptr),
    _tasks(nullptr),
    _labels(),
    _initRanges()
{
    // Offscreen buffer must be created in the UI thread because it is a QWindow, afterwards we move it
    _offscreenBuffer = new OffscreenBuffer();
}

TsneWorker::TsneWorker(TsneParameters tsneParameters, KnnParameters knnParameters, const std::vector<float>& data, uint32_t numDimensions, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges) :
    TsneWorker(tsneParameters)
{
    _knnParameters = knnParameters;
    assert(numDimensions > 0);
    _numPoints = data.size() / numDimensions;
    _numDimensions = numDimensions;
    _data = data;
    _embedding = { static_cast<uint32_t>(_tsneParameters.getNumDimensionsOutput()), _numPoints };
    _labels = labels;
    _initRanges = initRanges;

    if (initEmbedding)
        setInitEmbedding(*initEmbedding);
}

TsneWorker::TsneWorker(TsneParameters parameters, KnnParameters knnParameters, std::vector<float>&& data, uint32_t numDimensions, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges) :
    TsneWorker(parameters)
{
    _knnParameters = knnParameters;
    assert(numDimensions > 0);
    _numPoints = data.size() / numDimensions;
    _numDimensions = numDimensions;
    _data = std::move(data);
    _embedding = { static_cast<uint32_t>(_tsneParameters.getNumDimensionsOutput()), _numPoints };
    _labels = labels;
    _initRanges = initRanges;

    if (initEmbedding)
        setInitEmbedding(*initEmbedding);
}

TsneWorker::TsneWorker(TsneParameters parameters, const std::vector<hdi::data::MapMemEff<uint32_t, float>>& probDist, uint32_t numPoints, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges) :
    TsneWorker(parameters)
{
    _probabilityDistribution = probDist;
    _hasProbabilityDistribution = true;
    _numPoints = numPoints;
    _embedding = { static_cast<uint32_t>(_tsneParameters.getNumDimensionsOutput()), _numPoints };
    _labels = labels;
    _initRanges = initRanges;

    if (initEmbedding)
        setInitEmbedding(*initEmbedding);
}

TsneWorker::TsneWorker(TsneParameters parameters, std::vector<hdi::data::MapMemEff<uint32_t, float>>&& probDist, uint32_t numPoints, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges) :
    TsneWorker(parameters)
{
    _probabilityDistribution = std::move(probDist);
    _hasProbabilityDistribution = true;
    _numPoints = numPoints;
    _embedding = { static_cast<uint32_t>(_tsneParameters.getNumDimensionsOutput()), _numPoints };
    _tsneParameters.setExaggerationFactor(4 + _numPoints / 60000.0);
    _labels = labels;
    _initRanges = initRanges;

    if (initEmbedding)
        setInitEmbedding(*initEmbedding);
}

TsneWorker::~TsneWorker()
{
    delete _offscreenBuffer;
}

void TsneWorker::changeThread(QThread* targetThread)
{
    this->moveToThread(targetThread);
    
    //_task->moveToThread(targetThread);

    // Move the Offscreen buffer to the processing thread after creating it in the UI Thread
    _offscreenBuffer->moveToThread(targetThread);
    _offscreenBuffer->getContext()->moveToThread(targetThread);
}

void TsneWorker::resetThread()
{
    changeThread(QCoreApplication::instance()->thread());
}

int TsneWorker::getNumIterations() const
{
    return _currentIteration + 1;
}

void TsneWorker::setParentTask(mv::Task* parentTask)
{
    _parentTask = parentTask;

    //_tasks->getComputeGradientDescentTask().setGuiScopes({ Task::GuiScope::Foreground });
}

void TsneWorker::setInitEmbedding(const hdi::data::Embedding<float>::scalar_vector_type& initEmbedding)
{
    assert(initEmbedding.size() == _embedding.numDataPoints() * _embedding.numDimensions());
    _embedding.getContainer() = initEmbedding;
    _tsneParameters.setPresetEmbedding(true);
}

void TsneWorker::setCurrentIteration(int currentIteration)
{
    if(currentIteration < 0)
        return;
    
    _currentIteration = currentIteration;

    // we do not want to reapeat the exageration phase when continuing the gradient descent
    if (_currentIteration > (_tsneParameters.getExaggerationIter() + _tsneParameters.getExponentialDecayIter()))
    {
        _tsneParameters.setExaggerationFactor(1);
        _tsneParameters.setExaggerationIter(0);
        _tsneParameters.setExponentialDecayIter(0);
    }
    else if (_currentIteration > _tsneParameters.getExaggerationIter())
    {
        double decay = 1. - double(_currentIteration - _tsneParameters.getExaggerationIter()) / _tsneParameters.getExponentialDecayIter();
        _tsneParameters.setExaggerationFactor( 1 + (_tsneParameters.getExaggerationFactor() - 1) * decay );
    }
}

void TsneWorker::createTasks()
{
    _tasks = new TsneWorkerTasks(this, _parentTask);
}

hdi::dr::TsneParameters TsneWorker::tsneParameters()
{
    hdi::dr::TsneParameters tsneParameters;

    tsneParameters._embedding_dimensionality    = _tsneParameters.getNumDimensionsOutput();
    tsneParameters._mom_switching_iter          = _tsneParameters.getExaggerationIter();
    tsneParameters._remove_exaggeration_iter    = _tsneParameters.getExaggerationIter();
    tsneParameters._exaggeration_factor         = _tsneParameters.getExaggerationFactor();
    tsneParameters._exponential_decay_iter      = _tsneParameters.getExponentialDecayIter();
    tsneParameters._presetEmbedding             = _tsneParameters.getPresetEmbedding();

    tsneParameters._dimenfix = _tsneParameters.getDimenFix();
    tsneParameters._mode = _tsneParameters.getMode();
    tsneParameters._alpha = _tsneParameters.getAlpha();
    tsneParameters._beta = _tsneParameters.getBeta();
    tsneParameters._sigma = _tsneParameters.getSigma();
    tsneParameters._iters = _tsneParameters.getIters();
    tsneParameters._fix_selection = _tsneParameters.getFixSelection();
    tsneParameters._class_order = _tsneParameters.getClassOrder();
    tsneParameters._switch_axis = _tsneParameters.getSwitchAxis();

    return tsneParameters;
}

hdi::dr::HDJointProbabilityGenerator<float>::Parameters TsneWorker::probGenParameters()
{
    hdi::dr::HDJointProbabilityGenerator<float>::Parameters probGenParams;

    probGenParams._perplexity               = _tsneParameters.getPerplexity();
    probGenParams._perplexity_multiplier    = 3;
    probGenParams._num_trees                = _knnParameters.getAnnoyNumTrees();
    probGenParams._num_checks               = _knnParameters.getAnnoyNumChecks();
    probGenParams._aknn_algorithmP1         = _knnParameters.getHNSWm();
    probGenParams._aknn_algorithmP2         = _knnParameters.getHNSWef();
    probGenParams._aknn_algorithm           = _knnParameters.getKnnAlgorithm();
    probGenParams._aknn_metric              = _knnParameters.getKnnDistanceMetric();

    return probGenParams;
}

void TsneWorker::computeSimilarities()
{
    assert(_data.size() == _numDimensions * _numPoints);

    _tasks->getComputingSimilaritiesTask().setRunning();

    double t = 0.0;
    {
        hdi::utils::ScopedTimer<double> timer(t);

        _probabilityDistribution.clear();
        _probabilityDistribution.resize(_numPoints);
        qDebug() << "Sparse matrix allocated.";

        hdi::dr::HDJointProbabilityGenerator<float> probabilityGenerator;

        qDebug() << "Computing high dimensional probability distributions: Num dims: " << _numDimensions << " Num data points: " << _numPoints;
        probabilityGenerator.computeJointProbabilityDistribution(_data.data(), _numDimensions, _numPoints, _probabilityDistribution, probGenParameters());         // The _probabilityDistribution is symmetrized here.
    }
    
    qDebug() << "================================================================================";
    qDebug() << "tSNE: Computed probability distribution: " << t / 1000 << " seconds";
    qDebug() << "--------------------------------------------------------------------------------";

    _tasks->getComputingSimilaritiesTask().setFinished();
}

std::vector<hdi::dr::GpgpuSneCompute::Point2D> genRanges(std::string fix_sel, int num_points, std::vector<int> labels, float alpha, std::vector<float> _initRanges, bool density) {
    std::vector<hdi::dr::GpgpuSneCompute::Point2D> range_limits(num_points);
    if (fix_sel == "class_label") {
        std::map<int, int> label_counts;
        for (int label : labels)
        {
            label_counts[label]++;
        }

        std::map<int, hdi::dr::GpgpuSneCompute::Point2D> label_ranges;
        // TODO: take any input label type, convert to int
        float current_start = 0.0f;
        const float total_range = 100.0f;

        for (const auto& pair : label_counts)
        {
            int label = pair.first;
            int count = pair.second;
            float proportion;
            if (density) {
                proportion = static_cast<float>(count) / labels.size();
            }
            else {
                proportion = 1.0f / static_cast<float>(label_counts.size());
            }
            
            float range_size = proportion * total_range;

            label_ranges[label] = {current_start, current_start + range_size};
            current_start += range_size;
        }

        // apply alpha
        float max_r = -1.0f;
        float min_r = 1000000.f;
        for (const auto& pair : label_counts)
        {
            int label = pair.first;
            float r_l = label_ranges[label].x;
            float r_u = label_ranges[label].y;
            label_ranges[label].x = -(r_u - r_l) * alpha / 2.0f + (r_u + r_l) / 2.0f;
            label_ranges[label].y = (r_u - r_l) * alpha / 2.0f + (r_u + r_l) / 2.0f;
            max_r = std::max(max_r, label_ranges[label].y);
            min_r = std::min(min_r, label_ranges[label].x);
        }

        // normalize
        float range = max_r - min_r;
        for (const auto& pair : label_counts) {
            int label = pair.first;
            float normX = (label_ranges[label].x - min_r) / range * 100.0f;
            float normY = (label_ranges[label].y - min_r) / range * 100.0f;
            label_ranges[label].x = normX;
            label_ranges[label].y = normY;
        }

        // assign range to each point
        for (size_t i = 0; i < labels.size(); ++i)
        {
            int label = labels[i];
            range_limits[i] = label_ranges[label];
        }

        // debug info
        qDebug() << "Range limits created for" << labels.size() << "points, with" << label_counts.size() << "distinct labels.";
        

    }
    else if (fix_sel == "feature_value") {
        // actually only 1 input dimension is used
        std::set<float> values;
        for (size_t i = 0; i < _initRanges.size(); i += 2) {
            values.insert(_initRanges[i]);
        }

        std::unordered_map<float, hdi::dr::GpgpuSneCompute::Point2D> range_map;
        float max_r = FLT_MIN;
        float min_r = FLT_MAX;

        for (auto it = values.begin(); it != values.end(); ++it) {
            float cur_val = *it;
            float next_val;
            float last_val;
            if (it == values.begin()) {
                next_val = *(std::next(it));
                range_map[cur_val].x = cur_val - (next_val - cur_val) * alpha / 2;
                range_map[cur_val].y = cur_val + (next_val - cur_val) * alpha / 2;
            }
            else if (std::next(it) == values.end()) {
                last_val = *(std::prev(it));
                range_map[cur_val].x = cur_val - (cur_val - last_val) * alpha / 2;
                range_map[cur_val].y = cur_val + (cur_val - last_val) * alpha / 2;
            }
            else {
                next_val = *(std::next(it));
                last_val = *(std::prev(it));
                range_map[cur_val].x = cur_val - (cur_val - last_val) * alpha / 2;
                range_map[cur_val].y = cur_val + (next_val - cur_val) * alpha / 2;
            }
            min_r = std::min(min_r, range_map[cur_val].x);
            max_r = std::max(max_r, range_map[cur_val].y);
        }

        // assign and normalize
        float range = (max_r - min_r != 0) ? (max_r - min_r) : 1.0f;
        for (size_t i = 0; i < _initRanges.size(); i += 2) {
            float normX = (range_map[_initRanges[i]].x - min_r) / range * 100.0f;
            float normY = (range_map[_initRanges[i]].y - min_r) / range * 100.0f;
            range_limits[i/2].x = normX;
            range_limits[i/2].y = normY;
        }
        
    }
    else if (fix_sel == "input") {
        // apply alpha
        for (size_t i = 0; i < _initRanges.size(); i += 2) {
            float r_l = _initRanges[i];
            float r_u = _initRanges[i + 1];
            _initRanges[i] = -(r_u - r_l) * alpha / 2.0f + (r_u + r_l) / 2.0f;
            _initRanges[i + 1] = (r_u - r_l) * alpha / 2.0f + (r_u + r_l) / 2.0f;
        }

        // normalize
        auto minMax = std::minmax_element(_initRanges.begin(), _initRanges.end());
        float minVal = *minMax.first;
        float maxVal = *minMax.second;
        float range = (maxVal - minVal != 0) ? (maxVal - minVal) : 1.0f;
        
        for (size_t i = 0; i < _initRanges.size(); i += 2) {
            float normX = (_initRanges[i] - minVal) / range * 100.0f;
            float normY = (_initRanges[i + 1] - minVal) / range * 100.0f;
            range_limits[i/2].x = normX;
            range_limits[i/2].y = normY;
        }
    }
    return range_limits;
}

void TsneWorker::updateGPGPUSettings() {
    auto params = tsneParameters();
    // std::cout << params._switch_axis << std::endl;
    _GPGPU_tSNE.updateParams(params);
    std::vector<int> labels(_labels.size());
    std::transform(_labels.begin(), _labels.end(), labels.begin(), [](float val) {
        return static_cast<int>(val);  // Converts float to int (truncation)
    });
    std::string fix_sel = _tsneParameters.getFixSelection();
    std::vector<hdi::dr::GpgpuSneCompute::Point2D> range_limits = genRanges(fix_sel, labels.size(), labels, _tsneParameters.getAlpha(), _initRanges, _tsneParameters.getDensity());
    qDebug() << "First 10 range_limits:";
    for (size_t i = 0; i < std::min<size_t>(10, range_limits.size()); ++i)
    {
        qDebug() << "Point" << i << ": Lower =" << range_limits[i].x
                << ", Upper =" << range_limits[i].y;
    }
    _GPGPU_tSNE.updateArrays(range_limits, labels);
}

void TsneWorker::computeGradientDescent(uint32_t iterations)
{
    if (_shouldStop)
        return;

    const auto updateEmbedding = [this](const TsneData& tsneData) -> void {
        copyEmbeddingOutput();
        emit embeddingUpdate(tsneData);
        };

    auto initGPUTSNE = [this]() {
        // Initialize offscreen buffer
        double t_buffer = 0.0;
        {
            hdi::utils::ScopedTimer<double> timer(t_buffer);
            _offscreenBuffer->bindContext();
        }
        qDebug() << "tSNE: Set up offscreen buffer in " << t_buffer / 1000 << " seconds.";

        if (!_GPGPU_tSNE.isInitialized())
        {
            auto params = tsneParameters();

            // change _labels from float to int
            std::vector<int> labels(_labels.size());

            std::transform(_labels.begin(), _labels.end(), labels.begin(), [](float val) {
                return static_cast<int>(val);  // Converts float to int (truncation)
            });

            // choice 0: value (fix to exact value)
            // choice 1: density based for class labels
            // choice 2: input by user
            std::string fix_sel = _tsneParameters.getFixSelection();
            std::vector<hdi::dr::GpgpuSneCompute::Point2D> range_limits = genRanges(fix_sel, labels.size(), labels, _tsneParameters.getAlpha(), _initRanges, _tsneParameters.getDensity());
            

            qDebug() << "First 10 range_limits:";
            for (size_t i = 0; i < std::min<size_t>(10, range_limits.size()); ++i)
            {
                qDebug() << "Point" << i << ": Lower =" << range_limits[i].x
                        << ", Upper =" << range_limits[i].y;
            }
            

            // In case of HSNE, the _probabilityDistribution is a non-symmetric transition matrix and initialize() symmetrizes it here
            if (_hasProbabilityDistribution)
                _GPGPU_tSNE.initialize(_probabilityDistribution, &_embedding, params, range_limits, labels);
            else
                _GPGPU_tSNE.initializeWithJointProbabilityDistribution(_probabilityDistribution, &_embedding, params, range_limits, labels);

            qDebug() << "A-tSNE (GPU): Exaggeration factor: " << params._exaggeration_factor << ", exaggeration iterations: " << params._remove_exaggeration_iter << ", exaggeration decay iter: " << params._exponential_decay_iter;
        }
        updateGPGPUSettings();
    };

    auto initCPUTSNE = [this]() {
        if (!_CPU_tSNE.isInitialized())
        {
            auto params = tsneParameters();

            double theta = std::min(0.5, std::max(0.0, (_numPoints - 1000.0) * 0.00005));
            _CPU_tSNE.setTheta(theta);

            // In case of HSNE, the _probabilityDistribution is a non-summetric transition matrix and initialize() symmetrizes it here
            if (_hasProbabilityDistribution)
                _CPU_tSNE.initialize(_probabilityDistribution, &_embedding, params);
            else
                _CPU_tSNE.initializeWithJointProbabilityDistribution(_probabilityDistribution, &_embedding, params);

            qDebug() << "t-SNE (CPU, Barnes-Hut): Exaggeration factor: " << params._exaggeration_factor << ", exaggeration iterations: " << params._remove_exaggeration_iter << ", exaggeration decay iter: " << params._exponential_decay_iter << ", theta: " << theta;
        }
    };

    auto initTSNE = [this, initGPUTSNE, initCPUTSNE, updateEmbedding]() {
        double t_init = 0.0;
        {
            hdi::utils::ScopedTimer<double> timer(t_init);

            if (_tsneParameters.getGradientDescentType() == GradientDescentType::GPU)
                initGPUTSNE();
            else
                initCPUTSNE();

            updateEmbedding(_outEmbedding);
        }
        qDebug() << "tSNE: Init t-SNE " << t_init / 1000 << " seconds.";
    };

    auto singleTSNEIteration = [this]() {
        if (_tsneParameters.getGradientDescentType() == GradientDescentType::GPU)
            _GPGPU_tSNE.doAnIteration();
        else
            _CPU_tSNE.doAnIteration();
    };

    auto gradientDescentCleanup = [this]() {
        if (_tsneParameters.getGradientDescentType() == GradientDescentType::GPU)
            _offscreenBuffer->releaseContext();
        else
            return; // Nothing to do for CPU implementation
    };

    _tasks->getInitializeTsneTask().setRunning();

    initTSNE();

    _tasks->getInitializeTsneTask().setFinished();

    const auto beginIteration = _currentIteration;
    const auto endIteration = beginIteration + iterations;

    double elapsed = 0;
    double t_grad = 0;
    {
        qDebug() << "tSNE: Computing " << endIteration - beginIteration << " gradient descent iterations...";

        _tasks->getComputeGradientDescentTask().setRunning();
        _tasks->getComputeGradientDescentTask().setSubtasks(iterations);

        int currentStepIndex = 0;

        // Performs gradient descent for every iteration
        for (_currentIteration = beginIteration; _currentIteration < endIteration; ++_currentIteration) {

            _tasks->getComputeGradientDescentTask().setSubtaskStarted(currentStepIndex);

            hdi::utils::ScopedTimer<double> timer(t_grad);

            // Perform t-SNE iteration
            singleTSNEIteration();

            if (_currentIteration > 0 && _tsneParameters.getUpdateCore() > 0 && _currentIteration % _tsneParameters.getUpdateCore() == 0)
                updateEmbedding(_outEmbedding);

            if (t_grad > 1000)
                qDebug() << "Time: " << t_grad;

            elapsed += t_grad;

            // React to requests to stop
            if (_shouldStop)
                break;
            
            _tasks->getComputeGradientDescentTask().setSubtaskFinished(currentStepIndex);

            currentStepIndex++;

            QCoreApplication::processEvents();
        }

        gradientDescentCleanup();

        updateEmbedding(_outEmbedding);

        _tasks->getComputeGradientDescentTask().setFinished();
    }

    qDebug() << "--------------------------------------------------------------------------------";
    qDebug() << "tSNE: Finished embedding in: " << elapsed / 1000 << " seconds, with " << _currentIteration << " total iterations (" << endIteration - beginIteration << " new iterations)";
    qDebug() << "================================================================================";

    emit finished();
}

void TsneWorker::copyEmbeddingOutput()
{
    _outEmbedding.assign(_numPoints, _tsneParameters.getNumDimensionsOutput(), _embedding.getContainer());
}

void TsneWorker::updateParams(TsneParameters tsneParameters) {
    _tsneParameters = tsneParameters;
}

void TsneWorker::updateArrays(std::vector<float> initRanges, std::vector<float> labels)
{
    _initRanges = initRanges;
    _labels = labels;
}

void TsneWorker::compute()
{
    createTasks();

    connect(_parentTask, &Task::requestAbort, this, [this]() -> void { _shouldStop = true; }, Qt::DirectConnection);

    _shouldStop = false;

    double t = 0.0;
    {
        hdi::utils::ScopedTimer<double> timer(t);

        if (!_hasProbabilityDistribution)
            computeSimilarities();

        computeGradientDescent(_tsneParameters.getNumIterations());
    }
 
    qDebug() << "t-SNE total compute time: " << t / 1000 << " seconds.";

    if (_shouldStop)
        _tasks->getComputeGradientDescentTask().setAborted();
    else
        _tasks->getComputeGradientDescentTask().setFinished();

    _parentTask->setFinished();

    resetThread();
}

void TsneWorker::continueComputation(uint32_t iterations)
{
    _tasks->getInitializeOffScreenBufferTask().setEnabled(false);
    _tasks->getComputingSimilaritiesTask().setEnabled(false);
    _tasks->getInitializeTsneTask().setEnabled(false);
    
    connect(_parentTask, &Task::requestAbort, this, [this]() -> void { _shouldStop = true; }, Qt::DirectConnection);

    _shouldStop = false;

    updateGPGPUSettings();

    computeGradientDescent(iterations);

    _parentTask->setFinished();

    resetThread();
}

void TsneWorker::stop()
{
    _shouldStop = true;
}

TsneAnalysis::TsneAnalysis() :
    _tsneWorker(nullptr),
    _task(nullptr)
{
    qRegisterMetaType<TsneData>();
}

TsneAnalysis::~TsneAnalysis()
{
    _workerThread.quit();           // Signal the thread to quit gracefully
    if (!_workerThread.wait(500))   // Wait for the thread to actually finish
        _workerThread.terminate();  // Terminate thread after 0.5 seconds

    deleteWorker();
}

void TsneAnalysis::deleteWorker()
{
    if (_tsneWorker)
    {
        _tsneWorker->changeThread(QThread::currentThread());
        delete _tsneWorker;
    }
}

void TsneAnalysis::startComputation(TsneParameters parameters, const std::vector<hdi::data::MapMemEff<uint32_t, float>>& probDist, uint32_t numPoints, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, int previousIterations, std::vector<float> labels, std::vector<float> initRanges)
{
    deleteWorker();

    _tsneWorker = new TsneWorker(parameters, probDist, numPoints, initEmbedding, labels, initRanges);

    if (previousIterations >= 0)
        _tsneWorker->setCurrentIteration(previousIterations);

    startComputation();
}

void TsneAnalysis::startComputation(TsneParameters parameters, std::vector<hdi::data::MapMemEff<uint32_t, float>>&& probDist, uint32_t numPoints, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, int previousIterations, std::vector<float> labels, std::vector<float> initRanges)
{
    deleteWorker();

    _tsneWorker = new TsneWorker(parameters, std::move(probDist), numPoints, initEmbedding, labels, initRanges);

    if (previousIterations >= 0)
        _tsneWorker->setCurrentIteration(previousIterations);

    startComputation();
}

void TsneAnalysis::startComputation(TsneParameters parameters, KnnParameters knnParameters, const std::vector<float>& data, uint32_t numDimensions, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges)
{
    deleteWorker();

    _tsneWorker = new TsneWorker(parameters, knnParameters, data, numDimensions, initEmbedding, labels, initRanges);
    
    startComputation();
}

void TsneAnalysis::startComputation(TsneParameters parameters, KnnParameters knnParameters, std::vector<float>&& data, uint32_t numDimensions, const hdi::data::Embedding<float>::scalar_vector_type* initEmbedding, std::vector<float> labels, std::vector<float> initRanges)
{
    deleteWorker();

    _tsneWorker = new TsneWorker(parameters, knnParameters, std::move(data), numDimensions, initEmbedding, labels, initRanges);
    
    startComputation();
}

void TsneAnalysis::continueComputation(int iterations)
{
    if (!canContinue())
        return;

    _tsneWorker->changeThread(&_workerThread);

    emit continueWorker(iterations);
}

void TsneAnalysis::updateParams(TsneParameters tsneParameters)
{
    if (_tsneWorker)
    _tsneWorker->updateParams(tsneParameters);
}

void TsneAnalysis::updateArrays(std::vector<float> initRanges, std::vector<float> labels)
{
    if (_tsneWorker) {
        _tsneWorker->changeThread(&_workerThread);
        _tsneWorker->updateArrays(initRanges, labels);
    }
}

void TsneAnalysis::stopComputation()
{
    emit stopWorker();  // to _workerThread in Thread
    
    emit aborted();     // to external listeners
}

void TsneAnalysis::setTask(mv::Task* task)
{
    assert(task);
    _task = task;
}

void TsneAnalysis::setInitEmbedding(const hdi::data::Embedding<float>::scalar_vector_type& initEmbedding)
{
    if (_tsneWorker)
        _tsneWorker->setInitEmbedding(initEmbedding);
}

void TsneAnalysis::startComputation()
{
    _tsneWorker->setParentTask(_task);

    _tsneWorker->getOffscreenBuffer().initialize();
    _tsneWorker->changeThread(&_workerThread);
    
    // To-Worker signals
    connect(this, &TsneAnalysis::startWorker, _tsneWorker, &TsneWorker::compute);
    connect(this, &TsneAnalysis::continueWorker, _tsneWorker, &TsneWorker::continueComputation);
    connect(this, &TsneAnalysis::stopWorker, _tsneWorker, &TsneWorker::stop, Qt::DirectConnection);

    // From-Worker signals
    connect(_tsneWorker, &TsneWorker::embeddingUpdate, this, &TsneAnalysis::embeddingUpdate);
    connect(_tsneWorker, &TsneWorker::finished, this, &TsneAnalysis::finished);

    _workerThread.start();

    emit startWorker();
    emit started();
}

TsneWorkerTasks::TsneWorkerTasks(QObject* parent, mv::Task* parentTask) :
    QObject(parent),
    _initializeOffScreenBufferTask(this, "Initialize off-screen GPGPU buffer", Task::GuiScopes{ Task::GuiScope::DataHierarchy, Task::GuiScope::Foreground }, Task::Status::Idle),
    _computingSimilaritiesTask(this, "Compute similarities", Task::GuiScopes{ Task::GuiScope::DataHierarchy, Task::GuiScope::Foreground }, Task::Status::Idle),
    _initializeTsneTask(this, "Initialize TSNE", Task::GuiScopes{ Task::GuiScope::DataHierarchy, Task::GuiScope::Foreground }, Task::Status::Idle),
    _computeGradientDescentTask(this, "Compute gradient descent", Task::GuiScopes{ Task::GuiScope::DataHierarchy, Task::GuiScope::Foreground }, Task::Status::Idle)
{
    _initializeOffScreenBufferTask.setParentTask(parentTask);
    _computingSimilaritiesTask.setParentTask(parentTask);
    _initializeTsneTask.setParentTask(parentTask);
    _computeGradientDescentTask.setParentTask(parentTask);

    _computeGradientDescentTask.setWeight(20.f);
    _computeGradientDescentTask.setSubtaskNamePrefix("Compute gradient descent step");

    /*
    _initializeOffScreenBufferTask.moveToThread(targetThread);
    _computingSimilaritiesTask.moveToThread(targetThread);
    _initializeTsneTask.moveToThread(targetThread);
    _computeGradientDescentTask.moveToThread(targetThread);
    */
}
