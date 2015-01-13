// -*- C++ -*-

#ifndef ocropus_lstm__
#define ocropus_lstm__

#include <vector>
#include <string>
#include <iostream>
#include <typeinfo>
#include <memory>
#include <map>
#include <Eigen/Dense>

namespace ocropus {
using namespace std;
using Eigen::Ref;

#ifdef LSTM_DOUBLE
typedef double Float;
typedef Eigen::VectorXi iVec;
typedef Eigen::VectorXd Vec;
typedef Eigen::MatrixXd Mat;
#else
typedef float Float;
typedef Eigen::VectorXi iVec;
typedef Eigen::VectorXf Vec;
typedef Eigen::MatrixXf Mat;
#endif

typedef vector<Vec> Sequence;
typedef vector<int> Classes;

inline Vec timeslice(const Sequence &s, int i) {
    Vec result(s.size());
    for (int t = 0; t < s.size(); t++)
        result[t] = s[t][i];
    return result;
}

struct VecMat {
    Vec *vec = 0;
    Mat *mat = 0;
    VecMat() {
    }
    VecMat(Vec *vec) {
        this->vec = vec;
    }
    VecMat(Mat *mat) {
        this->mat = mat;
    }
};

struct INetwork {
    // Each network has a name that's used for loading
    // and saving.
    string name = "???";

    // Networks have input and output "ports" for sequences
    // and derivatives. These are propagated in forward()
    // and backward() methods.
    Sequence inputs, d_inputs;
    Sequence outputs, d_outputs;

    // Some networks have subnetworks. They should be
    // stored in the `sub` vector. That way, functions
    // like `save` can automatically traverse the tree
    // of networks. Together with the `name` field,
    // this forms a hierarchical namespace of networks.
    vector<shared_ptr<INetwork> > sub;

    // Learning rate and momentum used for training.
    Float lr = 1e-4;
    Float momentum = 0.9;

    // Data used for loading and saving networks; these
    // are generally only meaningful for the toplevel network.
    vector<int> codec;
    map<string, string> attributes;

    // Parameters specific to softmax.
    Float softmax_floor = 1e-5;
    bool softmax_accel = false;

    virtual ~INetwork() { }

    // There are several `init` methods depending on
    // the network; override the one that has the right
    // number of parameters.
    virtual void init(int no, int ni) {
        throw "unimplemented";
    }
    virtual void init(int no, int nh, int ni) {
        throw "unimplemented";
    }
    virtual void init(int no, int nh2, int nh, int ni) {
        throw "unimplemented";
    }

    // Main methods for forward and backward propagation
    // of activations.
    virtual void forward() = 0;
    virtual void backward() = 0;

    // Expected number of input/output features.
    virtual int ninput() { return -999999; }
    virtual int noutput() { return -999999; }

    // Add a network as a subnetwork.
    virtual void add(shared_ptr<INetwork> net) {
        sub.push_back(net);
    }

    // Hooks to iterate over the weights and states of this network.
    typedef function<void (const string &, VecMat, VecMat)> WeightFun;
    typedef function<void (const string &, Sequence *)> StateFun;
    virtual void myweights(const string &prefix, WeightFun f) { }
    virtual void mystates(const string &prefix, StateFun f) { }

    // Hooks executed prior to saving and after loading.
    // Loading iterates over the weights with the `weights`
    // methods and restores only the weights. `postLoad`
    // allows classes to update other internal state that
    // depends on matrix size.
    virtual void preSave() { }
    virtual void postLoad() { }

    // Set the learning rate for this network and all subnetworks.
    virtual void setLearningRate(Float lr, Float momentum) {
        this->lr = lr;
        this->momentum = momentum;
        for (int i = 0; i < sub.size(); i++)
            sub[i]->setLearningRate(lr, momentum);
    }

    // move the rest out of this class
    void setInputs(Sequence &inputs);
    void setTargets(Sequence &targets);
    void setTargetsAccelerated(Sequence &targets);
    void setClasses(Classes &classes);
    void train(Sequence &xs, Sequence &targets);
    void ctrain(Sequence &xs, Classes &cs);
    void ctrain_accelerated(Sequence &xs, Classes &cs, Float lo=1e-5);
    void cpred(Classes &preds, Sequence &xs);
    void info(string prefix);
    void weights(const string &prefix, WeightFun f);
    void states(const string &prefix, StateFun f);
    void networks(const string &prefix, function<void (string, INetwork*)>);
    Sequence *getState(string name);
    void save(const char *fname);
    void load(const char *fname);
};

INetwork *make_LinearLayer();
INetwork *make_LogregLayer();
INetwork *make_SoftmaxLayer();
INetwork *make_TanhLayer();
INetwork *make_ReluLayer();
INetwork *make_Stacked();
INetwork *make_Reversed();
INetwork *make_Parallel();
INetwork *make_MLP();
INetwork *make_LSTM();
INetwork *make_LSTM1();
INetwork *make_REVLSTM1();
INetwork *make_BIDILSTM();

void forward_algorithm(Mat &lr, Mat &lmatch, double skip=-5.0);
void forwardbackward(Mat &both, Mat &lmatch);
void ctc_align_targets(Sequence &posteriors, Sequence &outputs, Sequence &targets);
void ctc_align_targets(Sequence &posteriors, Sequence &outputs, Classes &targets);
void mktargets(Sequence &seq, Classes &targets, int ndim);

extern Mat debugmat;

namespace {
template <class A,class B>
double levenshtein(A &a,B &b) {
    int n = a.size();
    int m = b.size();
    if(n>m) return levenshtein(b,a);
    vector<double> current(n+1);
    vector<double> previous(n+1);
    for(int k=0;k<current.size();k++) current[k] = k;
    for(int i=1;i<=m;i++) {
        previous = current;
        for(int k=0;k<current.size();k++) current[k] = 0;
        current[0] = i;
        for(int j=1;j<=n;j++) {
            double add = previous[j]+1;
            double del = previous[j-1]+1;
            double change = previous[j-1];
            if(a[j-1]!=b[i-1]) change = change+1;
            current[j] = fmin(fmin(add,del),change);
        }
    }
    return current[n];
}
}


}

#endif
