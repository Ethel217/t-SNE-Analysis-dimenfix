#pragma once

enum class GradientDescentType
{
    GPU,
    CPU,
};


class TsneParameters
{
public:
    TsneParameters() :
        _numIterations(1000),
        _perplexity(30),
        _exaggerationIter(250),
        _exponentialDecayIter(150),
        _numDimensionsOutput(2),
        _presetEmbedding(false),
        _exaggerationFactor(4),
        _updateCore(10),
        _gradientDescentType(GradientDescentType::GPU),

        _dimenFix(true),
        _mode("clipping"),
        _iters(1),
        _fix_selection("class_label"),
        _class_order("random"),
        _alpha(1.0f),
        _beta(0.7f),
        _sigma(0.5f),
        _switch_axis(false),
        _density(true)
    {

    }

    void setNumIterations(int numIterations) { _numIterations = numIterations; }
    void setPerplexity(int perplexity) { _perplexity = perplexity; }
    void setExaggerationIter(int exaggerationIter) { _exaggerationIter = exaggerationIter; }
    void setExponentialDecayIter(int exponentialDecayIter) { _exponentialDecayIter = exponentialDecayIter; }
    void setNumDimensionsOutput(int numDimensionsOutput) { _numDimensionsOutput = numDimensionsOutput; }
    void setPresetEmbedding(bool presetEmbedding) { _presetEmbedding = presetEmbedding; }
    void setExaggerationFactor(double exaggerationFactor) { _exaggerationFactor = exaggerationFactor; }
    void setGradientDescentType(GradientDescentType gradientDescentType) { _gradientDescentType = gradientDescentType; }
    void setUpdateCore(int updateCore) { _updateCore = updateCore; }

    void setDimenfix(bool dimenFix) { _dimenFix = dimenFix; }
    void setDensity(bool density) { _density = density; }
    void setMode(std::string mode) { _mode = mode;  }
    void setAlpha(float alpha) { _alpha = alpha; }
    void setBeta(float beta) { _beta = beta; }
    void setSigma(float sigma) { _sigma = sigma; }
    void setIters(int iters) { _iters = iters; }
    void setFixSelection(std::string fix_selection) {_fix_selection = fix_selection;}
    void setClassOrder(std::string class_order) { _class_order = class_order; }
    void setSwitchAxis(bool switch_axis) { _switch_axis = switch_axis; }

    int getNumIterations() const { return _numIterations; }
    int getPerplexity() const { return _perplexity; }
    int getExaggerationIter() const { return _exaggerationIter; }
    int getExponentialDecayIter() const { return _exponentialDecayIter; }
    int getNumDimensionsOutput() const { return _numDimensionsOutput; }
    int getPresetEmbedding() const { return _presetEmbedding; }
    int getExaggerationFactor() const { return _exaggerationFactor; }
    GradientDescentType getGradientDescentType() const { return _gradientDescentType; }
    int getUpdateCore() const { return _updateCore; }

    bool getDimenFix() const { return _dimenFix; }
    bool getDensity() const { return _density; }
    int getIters() const { return _iters; }
    std::string getMode() const { return _mode; }
    std::string getFixSelection() const {return _fix_selection;}
    std::string getClassOrder() const { return _class_order; }
    bool getSwitchAxis() const { return _switch_axis; }
    float getAlpha() const { return _alpha; }
    float getBeta() const { return _beta; }
    float getSigma() const { return _sigma; }

private:
    int _numIterations;
    int _perplexity;
    int _exaggerationIter;
    int _exponentialDecayIter;
    int _numDimensionsOutput;
    double _exaggerationFactor;
    bool _presetEmbedding;
    GradientDescentType _gradientDescentType;     // Whether to use CPU or GPU gradient descent

    bool _dimenFix;
    bool _density;
    int _iters;
    std::string _mode;
    std::string _fix_selection;
    std::string _class_order;
    bool _switch_axis;
    float _alpha;
    float _beta;
    float _sigma;

    int _updateCore;        // Gradient descent iterations after which the embedding data set in ManiVault's core will be updated
};
