#include "../include/dnf_torch.h"

DNF::Net::Net(int nLayers, int nInput, bool withBias)
{
	// calc an exp reduction of the numbers always reaching 1
	const float b = (float)exp(log(nInput) / (nLayers - 1));
	int inputNeurons = nInput;
	for (int i = 1; i < nLayers; i++)
	{
		int outputNeurons = (int)ceil(nInput / pow(b, i));
		if (i == (nLayers - 1))
			outputNeurons = 1;
		char tmp[256];
		sprintf(tmp, "fc%d_%d_%d", i, inputNeurons, outputNeurons);
		if (debugOutput)
			fprintf(stderr, "Creating FC layer: %s\n", tmp);
		torch::nn::Linear ll = register_module(
			tmp,
			torch::nn::Linear(torch::nn::LinearOptions(inputNeurons, outputNeurons).bias(withBias)));
		torch::nn::init::xavier_uniform_(ll->weight, xavierGain);
		if (withBias)
			torch::nn::init::constant_(ll->bias, 0.0);
		fc.push_back(ll);
		if (1 == outputNeurons)
			break;
		inputNeurons = outputNeurons;
	}
}

torch::Tensor DNF::Net::forward(torch::Tensor x, ActMethod am)
{
	for (auto &f : fc)
	{
		switch (am)
		{
		default:
		case Act_Tanh:
			x = torch::atan(f->forward(x));
			break;
		case Act_Sigmoid:
			x = torch::sigmoid(f->forward(x));
			break;
		case Act_ReLU:
			x = torch::relu(f->forward(x));
			break;
		case Act_NONE:
			x = f->forward(x);
			break;
		}
	}
	return x;
}

DNF::DNF(const int nLayers,
		 const int nTaps,
		 const int signalDelayLineLength,
		 const std::string& model_filename,
		 const ActMethod am,
		 const bool tryGPU) : noiseDelayLineLength(nTaps),
							  signalDelayLineLength(signalDelayLineLength),
							  actMethod(am),
							  model(Net(nLayers, nTaps)),
							  optimizer(torch::optim::SGD(model.parameters(), 0))
{

	signal_delayLine.init(signalDelayLineLength);
	noise_delayLine.init(noiseDelayLineLength);

	if (tryGPU && torch::cuda::is_available())
	{
		const torch::DeviceType device_type = torch::kCUDA;
		device = torch::Device(device_type);
	}

	model.to(device);
	model.train();

	//load weights and biases from external file
	if(model_filename!=""){
		torch::serialize::InputArchive archive;
    	archive.load_from(model_filename);
    	model.load(archive);

	}

	//model.freezeAllButLast();
	

	saveInitialParameters();
}

float DNF::filter(const float signal, const float noise)
{
	if(!(learning_rate>0.0f)){
		torch::NoGradGuard no_grad;
	}
	const float delayed_signal = signal_delayLine.process(signal);
	noise_delayLine.process(noise);

	auto noiseTimeSeries = torch::zeros(noiseDelayLineLength, torch::kFloat);
	for (int i = 0; i < noiseDelayLineLength; i++)
	{
		noiseTimeSeries[i] = noise_delayLine.get(i);
	}
	noiseTimeSeries = noiseTimeSeries.to(device);

	torch::Tensor output = (model.forward(noiseTimeSeries, actMethod));
	remover = output.to(torch::kCPU).item<float>();

	f_nn = delayed_signal - remover;
	if(learning_rate>0.0f){
		optimizer.zero_grad();
		torch::Tensor gradient = torch::tensor({-f_nn}).to(device);
		output.retain_grad();
		output.backward(gradient);
		optimizer.step();
	}
	
	return f_nn;
}

float DNF::getWeightDistance() const
{
	auto dists = getLayerWeightDistances();
	float dsum = 0;
	for (const auto &dlayer : dists)
	{
		dsum = dsum + dlayer;
	}
	return dsum;
}

const std::vector<float> DNF::getLayerWeightDistances() const
{
	std::vector<float> distances;
	int i = 0;
	for (const auto &p : model.parameters())
	{
		torch::Tensor diff = (p - initialParameters[i]).view(-1);
		torch::Tensor dist = torch::norm(diff, 2);
		distances.push_back(dist.item<float>());
		i++;
	}
	return distances;
}

void DNF::setLearningRate(float mu)
{	
	learning_rate=mu;
	for (auto &group : optimizer.param_groups())
	{
		static_cast<torch::optim::SGDOptions &>(group.options()).lr(mu);
	}
}

void DNF::Net::save(torch::serialize::OutputArchive& archive) const
{
    for (size_t i = 0; i < fc.size(); ++i)
    {
        torch::serialize::OutputArchive layer_archive;
        fc[i]->save(layer_archive);
        archive.write("fc_" + std::to_string(i), layer_archive);
    }
}

void DNF::Net::load(torch::serialize::InputArchive& archive)
{
    for (size_t i = 0; i < fc.size(); ++i)
    {
        torch::serialize::InputArchive layer_archive;
        archive.read("fc_" + std::to_string(i), layer_archive);
        fc[i]->load(layer_archive);
    }
}

void DNF::Net::freezeAllButLast()
{
    // Freeze everything
    for (auto& p : parameters())
    {
        p.set_requires_grad(false);
    }

    // Unfreeze last FC layer
    for (auto& p : fc.back()->parameters())
    {
        p.set_requires_grad(true);
    }
	
}

void DNF::printModel() const
{
    const auto& model = getModel();

    size_t layer_idx = 0;
    for (const auto& layer : model.fc)
    {
        std::cout << "Layer " << layer_idx << " weights:\n";
        std::cout << layer->weight << "\n";

        if (layer->bias.defined())
        {
            std::cout << "Layer " << layer_idx << " bias:\n";
            std::cout << layer->bias << "\n";
        }

        std::cout << "----------------------------------\n";
        layer_idx++;
    }
}



