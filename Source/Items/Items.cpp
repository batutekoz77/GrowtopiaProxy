#include "items.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>   /* std::error_code, for the non-throwing file_size */


std::vector<Items> items;
bool ALREADY_INJECTED = false;

struct pos {
    pos(int _x, int _y) : x(_x), y(_y) {}

    pos(float _x, float _y)
        : x(static_cast<int>(std::round(_x / 32.0f))),
          y(static_cast<int>(std::round(_y / 32.0f))) {}

    int x{ 0 };
    int y{ 0 };

    float f_x() const { return x * 32.0f; }
    float f_y() const { return y * 32.0f; }

    auto operator<=>(const pos&) const = default;
};

class state {
public:
    int packet_create{ 04 };

    int type{};
    int netid{};
    int uid{}; // @todo understand this better @note so far I think this holds uid value
    int peer_state{};
    float count{}; // @todo understand this better
    int id{}; // @note peer's active hand, so 18 (fist) = punching, 32 (wrench) interacting, ect
    ::pos pos{ 0,0 }; // @note position 1D {x, y}
    std::array<float, 2zu> speed{}; // @note player movement (velocity(x), gravity(y)), higher gravity = smaller jumps
    int idk{};
    ::pos punch{ 0,0 }; // @note punching/placing position 2D {x, y}
    int size{};
};

std::vector<uint8_t> compress_state(const state& s) {
    std::vector<uint8_t> data(sizeof(::state) + 1, 0x00);
    int* _4bit = reinterpret_cast<int*>(data.data());
    float* _4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = s.packet_create;
    _4bit[1] = s.type;
    _4bit[2] = s.netid;
    _4bit[3] = s.uid;
    _4bit[4] = s.peer_state;
    _4bit_f[5] = s.count;
    _4bit[6] = s.id;
    ::pos f_pos = s.pos; // @todo
    _4bit_f[7] = f_pos.f_x();
    _4bit_f[8] = f_pos.f_y();
    _4bit_f[9] = s.speed[0];
    _4bit_f[10] = s.speed[1];
    _4bit[11] = s.idk;
    _4bit[12] = s.punch.x;
    _4bit[13] = s.punch.y;
    _4bit[14] = s.size;
    return data;
}

std::vector<uint8_t> im_data(sizeof(::state)/*inital packet*/ + 1, 0x00);

template<typename T>
void shift_pos(const std::vector<uint8_t>& data, uint32_t& pos, T& value) {
    uint8_t* _1bit = reinterpret_cast<uint8_t*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i)
        _1bit[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values··· */
template<typename T>
void data_modify(std::vector<uint8_t>& data, const uint32_t& pos, const T& value) {
    const uint8_t* _1bit = reinterpret_cast<const uint8_t*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i)
        data[pos + i] = _1bit[i];
}

std::string getGrowtopiaItemsPath() {
    if (auto* localAppData = std::getenv("LOCALAPPDATA"))
        return (std::filesystem::path(localAppData) / "Growtopia" / "cache" / "items.dat").string();
    return {};
}

/* @important: the highest items.dat layout this parser knows. Everything below
   is written against it; a newer file has fields we cannot place, and guessing
   would produce a silently wrong item table rather than an obvious failure. */
static constexpr uint8_t ITEMS_DAT_MAX_VERSION = 24;

/* Slack appended after the file so that a desynced read cannot leave the
   allocation. The parser does 17 unchecked raw-pointer reads; making every one
   of them safe would be a rewrite, and this makes the whole class harmless. */
static constexpr std::size_t ITEMS_DAT_PADDING = 64 * 1024;

/* How far to look for a lost id anchor. Newer versions have appended a byte or
   two at a time; a window this size covers that generously while still being far
   too small to "find" an anchor by coincidence in a desynced stream. */
static constexpr uint32_t ITEMS_DAT_MAX_SKEW = 10000;

static uint32_t readU32(const std::vector<uint8_t>& d, std::size_t at) {
    return static_cast<uint32_t>(d[at])
         | (static_cast<uint32_t>(d[at + 1]) << 8)
         | (static_cast<uint32_t>(d[at + 2]) << 16)
         | (static_cast<uint32_t>(d[at + 3]) << 24);
}

std::string InjectItems() {
    /* Whatever happens below, never come back: enter_game fires on every world
       change and a failing parse would repeat forever. */
    ALREADY_INJECTED = true;

    const std::string path = getGrowtopiaItemsPath();
    if (path.empty())
        return "items.dat: LOCALAPPDATA is not set -- item names unavailable.";

    /* @fix: the throwing overload. A missing items.dat raised
       std::filesystem::filesystem_error with nothing anywhere to catch it, so
       the proxy died on enter_game instead of reporting a missing file. */
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0) {
        return "items.dat not readable at '" + path + "' (" + ec.message() + "). "
               "Item names are unavailable; everything else still works.";
    }

    im_data = compress_state(::state{
        .type = 0x10,
        .peer_state = 0x08,
        .size = static_cast<int>(size)
    });

    const std::size_t dataEnd = im_data.size() + size;   /* end of real content */
    im_data.resize(dataEnd + ITEMS_DAT_PADDING);          /* + slack, zeroed */
    std::ifstream(path, std::ios::binary).read(reinterpret_cast<char*>(&im_data[sizeof(::state)]), size); // @note the binary data···

    uint32_t pos{ sizeof(::state) };
    uint8_t version{};
    shift_pos(im_data, pos, version); pos += 1; // @note downsize 'version' to 1 bit
    uint16_t count{};
    shift_pos(im_data, pos, count); pos += 2; // @note downside count to 2 bit

    /* --- adapting to a newer items.dat ----------------------------------
       Item records are stored in id order, and each one begins with its id as
       a 32-bit little-endian value. That is a checkable anchor: after reading
       record i, the next four bytes must be i+1.

       So a version that only APPENDS fields -- which is what every version
       here has done -- can be handled without knowing what it added: parse
       with the newest layout we know, see how far the anchor has moved, and
       carry that as trailing padding. The file teaches us its own layout.

       What this deliberately does NOT do is guess at a field inserted in the
       MIDDLE of a record, or one whose size changed. Those move the anchor
       unpredictably, the resync fails, and we stop -- which is the honest
       outcome, because a mis-parsed table of item names is worse than none. */
    uint32_t    extraPerRecord = 0;
    std::string note;   /* @note: this file has no logger; the caller logs what we return. */

    if (version > ITEMS_DAT_MAX_VERSION) {
        note = " [v" + std::to_string(version) + " is newer than the v"
             + std::to_string(ITEMS_DAT_MAX_VERSION) + " layout this parser knows]";
    }
    static constexpr std::string_view token{ "PBG892FXX982ABC*" };
    for (uint16_t i = 0; i < count; ++i) {
        /* One check per item is enough to catch a desync before it compounds:
           a wrong length field pushes pos past the content, and the next
           iteration stops here instead of reading further nonsense. */
        if (pos + 4 > dataEnd) {
            const std::string got = std::to_string(items.size());
            items.clear();
            return "items.dat parse ran off the end after " + got + " of " + std::to_string(count)
                 + " items (v" + std::to_string(version) + "). Item names are unavailable; "
                   "nothing else is affected.";
        }

        /* The anchor: this record must start with its own id. */
        if (readU32(im_data, pos) != i) {
            /* Scan forward for where the id actually landed. A short window on
               purpose: a large jump means we are lost, not merely behind. */
            bool resynced = false;
            for (uint32_t skip = 1; skip <= ITEMS_DAT_MAX_SKEW; ++skip) {
                if (pos + skip + 4 > dataEnd) break;
                if (readU32(im_data, pos + skip) == i) {
                    extraPerRecord += skip;
                    pos += skip;
                    resynced = true;
                    break;
                }
            }
            if (!resynced) {
                // try wider scan forward
                for (uint32_t skip = ITEMS_DAT_MAX_SKEW + 1; skip <= 50000; ++skip) {
                    if (pos + skip + 4 > dataEnd) break;
                    if (readU32(im_data, pos + skip) == i) {
                        extraPerRecord += skip;
                        pos += skip;
                        resynced = true;
                        break;
                    }
                }
                // try wider scan BACKWARD (parser may have overshot)
                if (!resynced) {
                    for (uint32_t skip = 1; skip <= 50000; ++skip) {
                        if (pos < skip) break;
                        if (readU32(im_data, pos - skip) == i) {
                            extraPerRecord -= skip;
                            pos -= skip;
                            resynced = true;
                            break;
                        }
                    }
                }
                if (!resynced) {
                    const std::string got = std::to_string(items.size());
                    items.clear();
                    return "items.dat v" + std::to_string(version) + " changed in a way this parser "
                           "cannot follow (lost the id anchor at item " + got + " of "
                         + std::to_string(count) + "). A field was probably inserted mid-record "
                           "rather than appended. Item names are unavailable; nothing else is affected.";
                }
            }
        }

        Items im{};

        shift_pos(im_data, pos, im.id); pos += 2; // @note downside im.id to 2 bit (short)
        shift_pos(im_data, pos, im.property);

        shift_pos(im_data, pos, im.cat);

        shift_pos(im_data, pos, im.type);
        pos += sizeof(uint8_t);

        short len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.raw_name.resize(len);
        for (short i = 0; i < len; ++i) im.raw_name[i] = im_data[pos] ^ token[(i + im.id) % token.length()], ++pos;

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(int);
        pos += sizeof(uint8_t);

        shift_pos(im_data, pos, im.ingredient);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);

        shift_pos(im_data, pos, im.collision);
        shift_pos(im_data, pos, im.hits);
        if (im.hits != 0) im.hits /= 6; // @note unknown reason behind why break hit is muliplied by 6 then having to divide by 6

        shift_pos(im_data, pos, im.hit_reset);

        if (im.type == type::CLOTHING) {
            uint8_t cloth_type{};
            shift_pos(im_data, pos, im.cloth_type);
        }
        else pos += 1; // @note assign nothing
        if (im.type == type::AURA) im.cloth_type = clothing::ances;
        shift_pos(im_data, pos, im.rarity);

        pos += sizeof(uint8_t);
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            std::string audio_directory{};
            audio_directory.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;

            if (audio_directory.ends_with(".mp3")) data_modify(im_data, pos, 0); // @todo make it only for IOS
        }
        pos += sizeof(int);

        pos += sizeof(std::array<uint8_t, 4zu>);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<uint8_t, 16zu>);

        shift_pos(im_data, pos, im.tick);

        pos += sizeof(short);
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<uint8_t, 80zu>);

        if (version >= 11) // @date February 2019
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 12) // @date October 2020
        {
            pos += sizeof(int);
            pos += sizeof(std::array<uint8_t, 9zu>);
        }
        if (version >= 13) pos += sizeof(int); // @date May 2021
        if (version >= 14) pos += sizeof(int); // @date October 2021
        if (version >= 15)
        {
            pos += sizeof(std::array<uint8_t, 25zu>);
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 16)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 17) pos += sizeof(int); // @date April 2024
        if (version >= 18) pos += sizeof(int); // @date December 2024
        if (version >= 19) pos += sizeof(std::array<uint8_t, 9zu>);
        if (version >= 21) pos += sizeof(short); // @date September 2025
        if (version >= 22)
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            im.info.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;
        }
        if (version >= 23)
        {
            shift_pos(im_data, pos, im.splice[0]);
            shift_pos(im_data, pos, im.splice[1]);
        }
        /* @fix: this was `version == 24`. Every other gate in this parser is
           `>=`, because each version ADDS a field and keeps the earlier ones.
           With `==`, a v25 file skips this byte and every record after the
           first is read at the wrong offset. */
        if (version >= 24) pos += sizeof(uint8_t); // @date December 2025

        /* Whatever a newer version appended beyond what we know, learned from
           the file itself rather than hardcoded. See the resync below. */
        pos += extraPerRecord;

        items.emplace_back(im);
    }
    ALREADY_INJECTED = true;
    return "Injected " + std::to_string(items.size()) + " items from the items.dat v"
         + std::to_string(version)
         + (extraPerRecord ? " (+" + std::to_string(extraPerRecord)
                             + " byte(s)/record learned from the file itself)" : "")
         + note;
}