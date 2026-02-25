#include "leduc/game.h"
#include "leduc/cfr.h"
#include "leduc/agent.h"

namespace fs = filesystem;

struct CFRConfig
{
    int num_iterations = 100000;
    int checkpoint_interval = 10000;
    string output_dir = "checkpoints";
    int seed = 42;
};

class CFRTrainer
{
public:
    CFRTrainer(const leduc::LeducGame &game, const CFRConfig &config)
        : game_(game), config_(config), solver_(game) {}

    void train()
    {
        cout << "Starting training with " << config_.num_iterations << " iterations\n";
        cout << "Checkpoint every " << config_.checkpoint_interval << " iterations\n";

        // Create output directory
        fs::create_directories(config_.output_dir);

        // Training loop
        for (int iter = 1; iter <= config_.num_iterations; ++iter)
        {
            solver_.train_iteration();

            if (iter % 1000 == 0)
            {
                cout << "Iteration " << iter << " / " << config_.num_iterations << "\n";
            }

            if (iter % config_.checkpoint_interval == 0)
            {
                save_checkpoint(iter);
            }
        }

        cout << "Training complete!\n";
    }

private:
    const leduc::LeducGame &game_;
    CFRConfig config_;
    leduc::VanillaCFR solver_;

    void save_checkpoint(int iteration)
    {
        string path = config_.output_dir + "/checkpoint_" + to_string(iteration) + ".json";
        cout << "Saving checkpoint: " << path << "\n";
        try
        {
            solver_.get_strategy_sum_table().save(path);
        }
        catch (const exception &e)
        {
            cerr << "Failed to save checkpoint: " << e.what() << "\n";
        }
    }
};

CFRConfig parse_args(int argc, char *argv[])
{
    CFRConfig config;
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--iterations" && i + 1 < argc)
        {
            config.num_iterations = stoi(argv[++i]);
        }
        else if (arg == "--output" && i + 1 < argc)
        {
            config.output_dir = argv[++i];
        }
        else if (arg == "--checkpoint-interval" && i + 1 < argc)
        {
            config.checkpoint_interval = stoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc)
        {
            config.seed = stoi(argv[++i]);
        }
    }
    return config;
}

int main(int argc, char *argv[])
{
    CFRConfig config = parse_args(argc, argv);

    cout << "=== Leduc Poker CFR Solver ===\n";
    cout << "Iterations: " << config.num_iterations << "\n";
    cout << "Output dir: " << config.output_dir << "\n\n";

    leduc::LeducGame game;
    CFRTrainer trainer(game, config);
    trainer.train();

    return 0;
}
