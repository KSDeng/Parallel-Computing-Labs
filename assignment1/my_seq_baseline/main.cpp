#include "main.h"
#include <sys/time.h>

Train::Train(unsigned id, MRT_LINE line, Station *location) {
    this->train_id = id;
    this->line = line;
    this->status = TRAIN_STATUS_INITIAL;
    this->station_at = location;
    this->direction = this->lineStationsManager()->isStartingStation(location) ? DIRECTION_FORWARD : DIRECTION_BACKWARD;
    this->load_passengers_counter = new TimeCounter();
    this->travel_link_counter = new TimeCounter();
}

void Train::enterPlatform(class Platform * plt) {
    this->setCurrentStatus(TRAIN_STATUS_IN_PLATFORM);
    this->station_at = plt->getStation();
    this->platform_at = plt;
    this->load_passengers_counter->setCounter(plt->getStation()->getPopularity());        // prepare for loading passengers

    plt->setOccupied(true);
}

void Train::enterPlatformQueue(class Platform * plt) {
    this->setCurrentStatus(TRAIN_STATUS_QUEUEING_FOR_PLATFORM);
    plt->addTrainToHoldingArea(this);
}

void Train::leavePlatform(class Platform * plt) {
    plt->setOccupied(false);

    if (!plt->holding_area.empty()) {       // notify trains in queue
        Train *firstTrain = plt->holding_area.front();
        firstTrain->enterPlatform(plt);
        plt->holding_area.pop_front();
    }
}

void Train::openDoor() {
    this->setCurrentStatus(TRAIN_STATUS_OPENING_DOOR);
}

void Train::loadPassengersFromStation(class Station * st) {
    this->setCurrentStatus(TRAIN_STATUS_LOADING_PASSENGERS);
    this->load_passengers_counter->count();
}


void Train::waitForAnotherTikToLink(class Link * link) {
    // last preparation before entering link
    this->setCurrentStatus(TRAIN_STATUS_WAITING_FOR_ANOTHER_TICK);
    this->travel_link_counter->setCounter(link->getDistance());       // prepare for transitioning
    link->setOccupied(true);
}

void Train::waitForLink() {
    this->setCurrentStatus(TRAIN_STATUS_WAITING_FOR_LINK);
}

void Train::enterLink(class Link * link) {
    this->setCurrentStatus(TRAIN_STATUS_TRANSITIONING);
    this->link_at = link;
    link->setOccupied(true);
}

void Train::leaveLink(class Link * link) {
    link->setOccupied(false);
}

void Train::transition() {
    this->travel_link_counter->count();
}

void Train::turnAround() {
    this->direction = (direction == DIRECTION_FORWARD) ? DIRECTION_BACKWARD : DIRECTION_FORWARD;
}

string Train::currentInfo() {
    string info;
    switch (this->line) {
        case MRT_LINE_GREEN: info += "g"; break;
        case MRT_LINE_YELLOW: info += "y"; break;
        case MRT_LINE_BLUE: info += "b"; break;
    }
    info += to_string(this->train_id);
    info += "-";
    if (this->status == TRAIN_STATUS_TRANSITIONING) {
        info += (this->link_at->getSrcStation()->getName());
        info += "->";
        info += (this->link_at->getDstStation()->getName());
    } else {
        info += (this->station_at->getName());
    }

    return info;
}

void spawnTrainsOnLine(int num, vector<Station*>& line_sts, MRT_LINE line, unsigned& id_counter, vector<Train*>& trains, vector<unsigned>& train_ids) {
    if (num == 1) {
        Train *train = new Train(id_counter++, line, line_sts[0]);
        Station *next_station = train->lineStationsManager()->getNextStationOfDirection(train->currentStation(), train->currentDirection());
        Platform *target_plt = train->currentStation()->getTargetPlatform(next_station);
        target_plt->isOccupied() ? train->enterPlatformQueue(target_plt) : train->enterPlatform(target_plt);

        trains.push_back(train);
        train_ids.push_back(train->getId());
    } else if (num == 2) {
        Train *train_at_start = new Train(id_counter++, line, line_sts[0]);
        Station *second_st = train_at_start->lineStationsManager()->getNextStationOfDirection(train_at_start->currentStation(), train_at_start->currentDirection());
        Platform *target_plt_of_train_at_start = train_at_start->currentStation()->getTargetPlatform(second_st);
        target_plt_of_train_at_start->isOccupied() ? train_at_start->enterPlatformQueue(target_plt_of_train_at_start) : train_at_start->enterPlatform(target_plt_of_train_at_start);

        trains.push_back(train_at_start);
        train_ids.push_back(train_at_start->getId());

        Train *train_at_terminal = new Train(id_counter++, line, line_sts[line_sts.size() - 1]);
        Station *second_last_st = train_at_terminal->lineStationsManager()->getNextStationOfDirection(train_at_terminal->currentStation(), train_at_terminal->currentDirection());
        Platform *target_plt_of_train_at_terminal = train_at_terminal->currentStation()->getTargetPlatform(second_last_st);
        target_plt_of_train_at_terminal->isOccupied() ? train_at_terminal->enterPlatformQueue(target_plt_of_train_at_terminal) : train_at_terminal->enterPlatform(target_plt_of_train_at_terminal);

        trains.push_back(train_at_terminal);
        train_ids.push_back(train_at_terminal->getId());
    }
}

void simulate(const unordered_map<string, Station*>& stations,
            size_t ticks,
            size_t num_green_trains,
            size_t num_yellow_trains,
            size_t num_blue_trains,
            size_t num_lines) {

    unsigned tick_counter = 0, train_id_counter = 0;
    unsigned cur_green_trains = 0, cur_yellow_trains = 0, cur_blue_trains = 0;

    vector<Train*> trains;
    vector<unsigned> green_train_ids, yellow_train_ids, blue_train_ids;

    while (tick_counter < ticks) {

        // spawn trains
        if (cur_green_trains + 2 <= num_green_trains) {
            spawnTrainsOnLine(2, green_line, MRT_LINE_GREEN, train_id_counter, trains, green_train_ids);
            cur_green_trains += 2;
        } else if (cur_green_trains + 1 <= num_green_trains) {
            spawnTrainsOnLine(1, green_line, MRT_LINE_GREEN, train_id_counter, trains, green_train_ids);
            cur_green_trains++;
        }

        if (cur_yellow_trains + 2 <= num_yellow_trains) {
            spawnTrainsOnLine(2, yellow_line, MRT_LINE_YELLOW, train_id_counter, trains, yellow_train_ids);
            cur_yellow_trains += 2;
        } else if (cur_yellow_trains + 1 <= num_yellow_trains) {
            spawnTrainsOnLine(1, yellow_line, MRT_LINE_YELLOW, train_id_counter, trains, yellow_train_ids);
            cur_yellow_trains++;
        }

        if (cur_blue_trains + 2 <= num_blue_trains) {
            spawnTrainsOnLine(2, blue_line, MRT_LINE_BLUE, train_id_counter, trains, blue_train_ids);
            cur_blue_trains += 2;
        } else if (cur_blue_trains + 1 <= num_blue_trains) {
            spawnTrainsOnLine(1, blue_line, MRT_LINE_BLUE, train_id_counter, trains, blue_train_ids);
            cur_blue_trains++;
        }

        for (Train* train: trains) {
            switch (train->currentStatus()) {
                case TRAIN_STATUS_INITIAL: {
                    // do nothing
                    break;
                }
                case TRAIN_STATUS_IN_PLATFORM: {
                    train->openDoor();
                    break;
                }
                case TRAIN_STATUS_QUEUEING_FOR_PLATFORM: {
                    // do nothing
                    break;
                }
                case TRAIN_STATUS_OPENING_DOOR: {
                    train->loadPassengersFromStation(train->currentStation());
                    break;
                }
                case TRAIN_STATUS_LOADING_PASSENGERS: {
                    if (train->getLoadingCounter()->finish()) {
                        Station *next_station = train->lineStationsManager()->getNextStationOfDirection(train->currentStation(), train->currentDirection());
                        Link *target_link = train->currentStation()->getTargetLink(next_station);

                        if (!target_link->isOccupied()) {
                            train->waitForAnotherTikToLink(target_link);
                        } else {
                            train->waitForLink();
                        }
                    } else {
                        train->loadPassengersFromStation(train->currentStation());
                    }
                    break;
                }
                case TRAIN_STATUS_WAITING_FOR_LINK: {
                    Station *next_station = train->lineStationsManager()->getNextStationOfDirection(train->currentStation(), train->currentDirection());
                    Link *target_link = train->currentStation()->getTargetLink(next_station);
                    if (!target_link->isOccupied()) {
                        train->waitForAnotherTikToLink(target_link);
                    }
                    break;
                }

                case TRAIN_STATUS_WAITING_FOR_ANOTHER_TICK: {
                    Station *next_station = train->lineStationsManager()->getNextStationOfDirection(train->currentStation(), train->currentDirection());
                    Link *target_link = train->currentStation()->getTargetLink(next_station);
                    train->leavePlatform(train->currentPlatform());
                    train->enterLink(target_link);
                    train->transition();
                    break;
                }

                case TRAIN_STATUS_TRANSITIONING: {
                    if (train->getTravelingCounter()->finish()) {
                        Station *dst_station = train->currentLink()->getDstStation();
                        if ((train->currentDirection() == DIRECTION_FORWARD && train->lineStationsManager()->isTerminalStation(dst_station))
                            || (train->currentDirection() == DIRECTION_BACKWARD && train->lineStationsManager()->isStartingStation(dst_station))) {
                            train->turnAround();        // train turn around
                        }
                        Station *next_st_of_dst_st = train->lineStationsManager()->getNextStationOfDirection(dst_station, train->currentDirection());
                        Platform *target_platform = dst_station->getTargetPlatform(next_st_of_dst_st);

                        train->leaveLink(train->currentLink());
                        if (target_platform->isOccupied()) {
                            train->enterPlatformQueue(target_platform);
                        } else {
                            train->enterPlatform(target_platform);
                            train->openDoor();
                        }
                    } else {
                        train->transition();
                    }
                    break;
                }

                default: cout<<"Unexpected status of train id "<<train->getId()<<endl; break;
            }
        }

        if (tick_counter >= ticks - num_lines) {    // print info
            string info;
            info += (to_string(tick_counter) + ": ");
            for (unsigned b_id: blue_train_ids) {
                info += trains[b_id]->currentInfo();
                info += " ";
            }
            for (unsigned g_id: green_train_ids) {
                info += trains[g_id]->currentInfo();
                info += " ";
            }
            for (unsigned y_id: yellow_train_ids) {
                info += trains[y_id]->currentInfo();
                info += " ";
            }
            info = info.substr(0, info.size() - 1);
            cout<<info<<endl;
        }

        tick_counter++;
    }

}

long long wall_clock_time()
{
#ifdef LINUX
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
#endif
}

int main(int argc, char const* argv[]) {

    if (argc < 2) {
        cerr << argv[0] << " <input_file>\n";
        exit(1);
    }

    ifstream ifs(argv[1], std::ios_base::in);
    if (!ifs.is_open()) {
        cerr << "Failed to open " << argv[1] << '\n';
        exit(2);
    }

    // Read S
    size_t S;
    ifs >> S;

    // Read station names.
    string station_name;
    vector<string> st_names;
    st_names.reserve(S);
    unordered_map<string, Station*> stations;

    for (size_t i = 0; i < S; ++i) {
        ifs >> station_name;
        st_names.emplace_back(station_name);

        auto *st = new Station(station_name);
        stations[station_name] = st;
    }

    // Read P popularity
    size_t p;
    for (size_t i = 0; i < S; ++i) {
        ifs >> p;
        stations[st_names[i]]->setPop(p);
    }

    // Construct links from adjacency mat
    vector<Link*> links;
    size_t distance;
    for (size_t src{}; src < S; ++src) {
        for (size_t dst{}; dst < S; ++dst) {
            ifs >> distance;
            if (distance > 0) {
                Station *st_src = stations[st_names[src]];
                Station *st_dst = stations[st_names[dst]];
                Link *link = new Link(st_src, st_dst, distance);
                links.emplace_back(link);       // TODO: emplace_back和push_back有什么区别？
                st_src->addLinkTowards(st_dst, link);
            }
        }
    }

    ifs.ignore();

    // Read station names of different lines

    string stations_buf;
    getline(ifs, stations_buf);
    stringstream ss_green(stations_buf);
    string green_station_name;
    while (ss_green >> green_station_name) {
        green_line.push_back(stations[green_station_name]);
    }

    getline(ifs, stations_buf);
    stringstream ss_yellow(stations_buf);
    string yellow_station_name;
    while (ss_yellow >> yellow_station_name) {
        yellow_line.push_back(stations[yellow_station_name]);
    }

    getline(ifs, stations_buf);
    stringstream ss_blue(stations_buf);
    string blue_station_name;
    while (ss_blue >> blue_station_name) {
        blue_line.push_back(stations[blue_station_name]);
    }

    green_line_manager = new LineStationsManager(MRT_LINE_GREEN);
    yellow_line_manager = new LineStationsManager(MRT_LINE_YELLOW);
    blue_line_manager = new LineStationsManager(MRT_LINE_BLUE);

    // N time ticks
    size_t N;
    ifs >> N;

    // g,y,b number of trains per line
    size_t g, y, b;
    ifs >> g;
    ifs >> y;
    ifs >> b;

    size_t num_lines;
    ifs >> num_lines;

    long long before, after;
    before = wall_clock_time();
    simulate(stations, N, g, y, b, num_lines);
    after = wall_clock_time();
    printf("%f seconds\n", ((float)(after - before)) / 1000000000);

    return 0;
}
