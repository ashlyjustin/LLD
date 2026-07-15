#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <string>
#include <algorithm>
#include <climits>
using namespace std;

// ── Enums ──────────────────────────────────────────────────────────────────

enum class SeatType     { REGULAR, PREMIUM, VIP };
enum class SeatState    { AVAILABLE, BOOKED };
enum class BookingStatus{ CONFIRMED, CANCELLED };

static string seatTypeToString(SeatType t) {
    switch (t) {
        case SeatType::REGULAR: return "REGULAR";
        case SeatType::PREMIUM: return "PREMIUM";
        case SeatType::VIP:     return "VIP";
    }
    return "UNKNOWN";
}

// Price is a property of the seat type (can be made show/screen-specific).
static double priceFor(SeatType t) {
    switch (t) {
        case SeatType::REGULAR: return 150.0;
        case SeatType::PREMIUM: return 250.0;
        case SeatType::VIP:     return 400.0;
    }
    return 0.0;
}

// ── Seat (physical layout, immutable after screen creation) ────────────────

struct Seat {
    int row, col;
    SeatType type;

    Seat() = default;
    Seat(int r, int c, SeatType t) : row(r), col(c), type(t) {}

    double price()  const { return priceFor(type); }
    string id()     const { return to_string(row) + ":" + to_string(col); }
};

// ── Screen (physical layout only, no booking state) ────────────────────────

class Screen {
public:
    string screenId;
    vector<vector<Seat>> seats; // [row][col]

    // Rows are divided into thirds: first → REGULAR, middle → PREMIUM, last → VIP
    Screen(string id, int rows, int cols) : screenId(move(id)), seats(rows, vector<Seat>(cols)) {
        int third = max(1, rows / 3);
        for (int r = 0; r < rows; ++r) {
            SeatType type = (r < third)         ? SeatType::REGULAR :
                            (r < 2 * third)     ? SeatType::PREMIUM :
                                                  SeatType::VIP;
            for (int c = 0; c < cols; ++c)
                seats[r][c] = Seat(r, c, type);
        }
    }

    int rows() const { return (int)seats.size(); }
    int cols() const { return seats.empty() ? 0 : (int)seats[0].size(); }

    SeatType seatTypeAt(int r, int c) const { return seats[r][c].type; }
    double   seatPriceAt(int r, int c) const { return seats[r][c].price(); }
};

// ── Movie ──────────────────────────────────────────────────────────────────

struct Movie {
    string id, title, genre;
    int durationMins;

    Movie() = default;
    Movie(string i, string t, string g, int d)
        : id(move(i)), title(move(t)), genre(move(g)), durationMins(d) {}
};

// ── Show (per-show booking state decoupled from Screen) ────────────────────

class Show {
    vector<vector<SeatState>> seatStates; // [row][col], per-show

public:
    string  showId;
    Movie*  movie;
    Screen* screen;
    string  showTime; // "YYYY-MM-DD HH:MM"

    Show(string sid, Movie* m, Screen* sc, string time)
        : seatStates(sc->rows(), vector<SeatState>(sc->cols(), SeatState::AVAILABLE)),
          showId(move(sid)), movie(m), screen(sc), showTime(move(time)) {}

    int rows() const { return screen->rows(); }
    int cols() const { return screen->cols(); }

    SeatType seatTypeAt(int r, int c)  const { return screen->seatTypeAt(r, c); }
    double   seatPriceAt(int r, int c) const { return screen->seatPriceAt(r, c); }

    bool isAvailable(int r, int c) const {
        return seatStates[r][c] == SeatState::AVAILABLE;
    }
    bool isAvailableOfType(int r, int c, SeatType type) const {
        return isAvailable(r, c) && seatTypeAt(r, c) == type;
    }

    void book(int r, int c)   { seatStates[r][c] = SeatState::BOOKED;     }
    void unbook(int r, int c) { seatStates[r][c] = SeatState::AVAILABLE;  }

    int countAvailable(SeatType type) const {
        int cnt = 0;
        for (int r = 0; r < rows(); ++r)
            for (int c = 0; c < cols(); ++c)
                if (isAvailableOfType(r, c, type)) ++cnt;
        return cnt;
    }
};

// ── Booking ────────────────────────────────────────────────────────────────

struct Booking {
    string         bookingId;
    string         userId;
    Show*          show;
    vector<pair<int,int>> seats; // (row, col) positions
    double         totalAmount;
    BookingStatus  status;

    Booking() = default;
    Booking(string bid, string uid, Show* s, vector<pair<int,int>> sp, double amt)
        : bookingId(move(bid)), userId(move(uid)), show(s),
          seats(move(sp)), totalAmount(amt), status(BookingStatus::CONFIRMED) {}

    void print() const {
        cout << "Booking[" << bookingId << "]"
             << " User=" << userId
             << " Show=" << show->showId
             << " (" << show->movie->title << " @" << show->showTime << ")"
             << " Seats=[";
        for (size_t i = 0; i < seats.size(); ++i) {
            if (i) cout << ',';
            cout << seats[i].first << ":" << seats[i].second
                 << "(" << seatTypeToString(show->seatTypeAt(seats[i].first, seats[i].second)) << ")";
        }
        cout << "] Total=Rs." << totalAmount
             << " [" << (status == BookingStatus::CONFIRMED ? "CONFIRMED" : "CANCELLED") << "]\n";
    }
};

// ── Seat Selection Strategy ────────────────────────────────────────────────
// Each strategy picks exactly `count` seats of `type` from the show.
// BestAvailableStrategy is the exception: it explicitly ignores `type` and
// upgrades/downgrades to find the best available block.

class SeatSelectionStrategy {
public:
    virtual vector<pair<int,int>> selectSeats(Show* show, int count, SeatType type) = 0;
    virtual ~SeatSelectionStrategy() = default;
};

// Scan rows front-to-back; within each row find the first consecutive block of `count`.
class ConsecutiveNearestStrategy : public SeatSelectionStrategy {
public:
    vector<pair<int,int>> selectSeats(Show* show, int count, SeatType type) override {
        for (int r = 0; r < show->rows(); ++r) {
            int run = 0, run_start = -1;
            for (int c = 0; c < show->cols(); ++c) {
                if (show->isAvailableOfType(r, c, type)) {
                    if (run == 0) run_start = c;
                    if (++run == count) {
                        vector<pair<int,int>> res;
                        for (int k = run_start; k < run_start + count; ++k)
                            res.push_back({r, k});
                        return res;
                    }
                } else {
                    run = 0; run_start = -1;
                }
            }
        }
        return {};
    }
};

// For each eligible row (sorted by closeness to the middle row), find the consecutive
// block of `count` seats whose center is nearest to the center column.
class CenterSeatsStrategy : public SeatSelectionStrategy {
public:
    vector<pair<int,int>> selectSeats(Show* show, int count, SeatType type) override {
        int midRow = show->rows() / 2;
        int midCol = show->cols() / 2;

        // Collect rows of the requested type sorted by distance to midRow
        vector<int> typeRows;
        for (int r = 0; r < show->rows(); ++r)
            if (show->seatTypeAt(r, 0) == type)
                typeRows.push_back(r);
        sort(typeRows.begin(), typeRows.end(), [&](int a, int b){
            return abs(a - midRow) < abs(b - midRow);
        });

        for (int r : typeRows) {
            int best_start = -1, best_dist = INT_MAX, run = 0;
            for (int c = 0; c <= show->cols(); ++c) {
                bool avail = (c < show->cols()) && show->isAvailableOfType(r, c, type);
                if (avail) {
                    ++run;
                    if (run >= count) {
                        int ws = c - count + 1;
                        int center = ws + (count - 1) / 2;
                        int dist = abs(center - midCol);
                        if (dist < best_dist) { best_dist = dist; best_start = ws; }
                    }
                } else {
                    run = 0;
                }
            }
            if (best_start >= 0) {
                vector<pair<int,int>> res;
                for (int k = best_start; k < best_start + count; ++k)
                    res.push_back({r, k});
                return res;
            }
        }
        return {};
    }
};

// Ignores the requested `type`; tries VIP → PREMIUM → REGULAR using center selection.
// Use this strategy when the caller explicitly opts into "best available upgrade" behaviour.
class BestAvailableStrategy : public SeatSelectionStrategy {
    CenterSeatsStrategy inner;
public:
    vector<pair<int,int>> selectSeats(Show* show, int count, SeatType /*preferredType*/) override {
        for (SeatType t : {SeatType::VIP, SeatType::PREMIUM, SeatType::REGULAR}) {
            auto seats = inner.selectSeats(show, count, t);
            if (!seats.empty()) return seats;
        }
        return {};
    }
};

// ── BookingSystem ──────────────────────────────────────────────────────────

class BookingSystem {
    unordered_map<string, unique_ptr<Movie>>   movies;
    unordered_map<string, unique_ptr<Screen>>  screens;
    unordered_map<string, unique_ptr<Show>>    shows;
    unordered_map<string, Booking>             bookings;
    unique_ptr<SeatSelectionStrategy>          strategy;
    int bookingCounter = 0;

    string nextBookingId() { return "BKG-" + to_string(++bookingCounter); }

public:
    explicit BookingSystem(unique_ptr<SeatSelectionStrategy> strat)
        : strategy(move(strat)) {}

    Movie* addMovie(string id, string title, string genre, int durationMins) {
        auto m = make_unique<Movie>(id, title, genre, durationMins);
        auto* ptr = m.get();
        movies[id] = move(m);
        return ptr;
    }

    Screen* addScreen(string id, int rows, int cols) {
        auto sc = make_unique<Screen>(id, rows, cols);
        auto* ptr = sc.get();
        screens[id] = move(sc);
        return ptr;
    }

    Show* addShow(const string& showId, const string& movieId,
                  const string& screenId, string time) {
        auto* m  = movies.count(movieId)  ? movies[movieId].get()   : nullptr;
        auto* sc = screens.count(screenId)? screens[screenId].get() : nullptr;
        if (!m || !sc) return nullptr;
        auto sh = make_unique<Show>(showId, m, sc, move(time));
        auto* ptr = sh.get();
        shows[showId] = move(sh);
        return ptr;
    }

    // Returns bookingId on success, nullopt if no seats available.
    optional<string> bookSeats(const string& userId, const string& showId,
                                int count, SeatType type) {
        auto it = shows.find(showId);
        if (it == shows.end()) return nullopt;
        Show* show = it->second.get();

        auto chosen = strategy->selectSeats(show, count, type);
        if (chosen.empty()) return nullopt;

        double total = 0;
        for (auto [r, c] : chosen) {
            show->book(r, c);
            total += show->seatPriceAt(r, c);
        }

        string bid = nextBookingId();
        bookings.emplace(bid, Booking(bid, userId, show, chosen, total));
        return bid;
    }

    bool cancelBooking(const string& bookingId) {
        auto it = bookings.find(bookingId);
        if (it == bookings.end()) return false;
        Booking& bkg = it->second;
        if (bkg.status == BookingStatus::CANCELLED) return false;
        for (auto [r, c] : bkg.seats)
            bkg.show->unbook(r, c);
        bkg.status = BookingStatus::CANCELLED;
        return true;
    }

    const Booking* getBooking(const string& bookingId) const {
        auto it = bookings.find(bookingId);
        return it != bookings.end() ? &it->second : nullptr;
    }

    vector<Show*> getShowsForMovie(const string& movieId) const {
        vector<Show*> res;
        for (auto& [id, sh] : shows)
            if (sh->movie->id == movieId) res.push_back(sh.get());
        return res;
    }

    void printShowStatus(const string& showId) const {
        auto it = shows.find(showId);
        if (it == shows.end()) { cout << "Show not found\n"; return; }
        Show* sh = it->second.get();

        cout << "Show: " << sh->showId << " | " << sh->movie->title
             << " | " << sh->showTime << " | Screen: " << sh->screen->screenId << "\n";
        for (int r = 0; r < sh->rows(); ++r) {
            for (int c = 0; c < sh->cols(); ++c) {
                if (!sh->isAvailable(r, c)) {
                    cout << "[X]";
                } else {
                    char sym = sh->seatTypeAt(r, c) == SeatType::VIP     ? 'V' :
                               sh->seatTypeAt(r, c) == SeatType::PREMIUM ? 'P' : 'R';
                    cout << '[' << sym << ']';
                }
            }
            cout << '\n';
        }
        cout << "Available — REGULAR:" << sh->countAvailable(SeatType::REGULAR)
             << "  PREMIUM:" << sh->countAvailable(SeatType::PREMIUM)
             << "  VIP:"     << sh->countAvailable(SeatType::VIP) << "\n";
    }
};

// ── Demo ───────────────────────────────────────────────────────────────────

static void runDemo(const string& stratName, unique_ptr<SeatSelectionStrategy> strat) {
    cout << "\n========== " << stratName << " ==========\n";
    BookingSystem sys(move(strat));

    sys.addMovie("M1", "Inception",        "Sci-Fi",  148);
    sys.addMovie("M2", "The Dark Knight",  "Action",  152);

    // 9 rows × 10 cols → rows 0-2: REGULAR, 3-5: PREMIUM, 6-8: VIP
    sys.addScreen("SC1", 9, 10);
    sys.addScreen("SC2", 9, 10);

    sys.addShow("SH1", "M1", "SC1", "2026-04-18 18:00");
    sys.addShow("SH2", "M2", "SC2", "2026-04-18 21:00");

    auto bid1 = sys.bookSeats("Alice", "SH1", 3, SeatType::PREMIUM);
    if (bid1) sys.getBooking(*bid1)->print();
    else      cout << "Alice: no PREMIUM seats available\n";

    auto bid2 = sys.bookSeats("Bob",   "SH1", 2, SeatType::VIP);
    if (bid2) sys.getBooking(*bid2)->print();
    else      cout << "Bob: no VIP seats available\n";

    auto bid3 = sys.bookSeats("Carol", "SH1", 4, SeatType::REGULAR);
    if (bid3) sys.getBooking(*bid3)->print();
    else      cout << "Carol: no REGULAR seats available\n";

    // Cancel Alice's booking and let Dave rebook those seats
    if (bid1) {
        bool ok = sys.cancelBooking(*bid1);
        cout << "Cancel " << *bid1 << ": " << (ok ? "OK" : "FAIL") << "\n";
    }
    auto bid4 = sys.bookSeats("Dave",  "SH1", 3, SeatType::PREMIUM);
    if (bid4) sys.getBooking(*bid4)->print();
    else      cout << "Dave: no PREMIUM seats available\n";

    cout << "\n";
    sys.printShowStatus("SH1");

    // Show both shows available for Inception
    cout << "\nShows for Inception:\n";
    for (Show* sh : sys.getShowsForMovie("M1"))
        cout << "  " << sh->showId << " @ " << sh->showTime << "\n";
}

int main() {
    runDemo("ConsecutiveNearestStrategy", make_unique<ConsecutiveNearestStrategy>());
    runDemo("CenterSeatsStrategy",        make_unique<CenterSeatsStrategy>());
    runDemo("BestAvailableStrategy",      make_unique<BestAvailableStrategy>());
    return 0;
}
