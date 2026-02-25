#include "leduc/game.h"
#include "leduc/agent.h"
#include "leduc/cfr.h"
#include "leduc/strategy.h"

class CLIGame
{
public:
    CLIGame(const leduc::LeducGame &game, leduc::Agent &agent)
        : game_(game), agent_(agent) {}

    void play_game(int human_player)
    {
        cout << "=== Playing Game ===\n";
        cout << "Human player: " << human_player << "\n";

        leduc::LeducState state = game_.initial_state();

        // Note: implement full game loop here
        // For now, stub
        throw runtime_error("Not implemented in Phase 8 - CLI game loop");
    }

    void play_session(int num_games)
    {
        cout << "Playing " << num_games << " games\n";
        for (int i = 0; i < num_games; ++i)
        {
            cout << "\n--- Game " << (i + 1) << " ---\n";
            // play_game(0 or 1);
        }
    }

private:
    const leduc::LeducGame &game_;
    leduc::Agent &agent_;

    void display_state(const leduc::LeducState &state, int human_player)
    {
        cout << "Current state:\n";
        cout << "  Round: " << state.current_round << "\n";
        cout << "  Pot: " << state.pot << "\n";
        cout << "  To call: " << state.chips_to_call << "\n";
    }

    string get_human_action()
    {
        string action;
        cout << "Enter action: ";
        cin >> action;
        return action;
    }
};

struct CLIConfig
{
    string checkpoint_path;
    int num_games = 1;
    int human_player = 0;
};

CLIConfig parse_args(int argc, char *argv[])
{
    CLIConfig config;
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--checkpoint" && i + 1 < argc)
        {
            config.checkpoint_path = argv[++i];
        }
        else if (arg == "--games" && i + 1 < argc)
        {
            config.num_games = stoi(argv[++i]);
        }
        else if (arg == "--human-player" && i + 1 < argc)
        {
            config.human_player = stoi(argv[++i]);
        }
    }
    return config;
}

int main(int argc, char *argv[])
{
    CLIConfig config = parse_args(argc, argv);

    cout << "=== Leduc Poker - Interactive Play ===\n";
    cout << "Checkpoint: " << config.checkpoint_path << "\n";
    cout << "Games: " << config.num_games << "\n";
    cout << "Human player: " << config.human_player << "\n\n";

    try
    {
        leduc::LeducGame game;
        leduc::InfoSetTable strategy = leduc::load_strategy(config.checkpoint_path);
        leduc::CFRAgent agent(strategy);

        CLIGame cli_game(game, agent);
        cli_game.play_session(config.num_games);
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
