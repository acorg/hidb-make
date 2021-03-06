#include <iostream>

#include "acmacs-base/string-join.hh"
#include "hidb-5/hidb-bin.hh"

// ----------------------------------------------------------------------

std::string hidb::bin::signature()
{
    return {"HIDB0500", 8};

} // hidb::bin::signature

// ----------------------------------------------------------------------

bool hidb::bin::has_signature(const char* data)
{
    const std::string sig = signature();
    return !std::memcmp(data, sig.data(), sig.size());

} // hidb::bin::has_signature

// ----------------------------------------------------------------------

std::string hidb::bin::Antigen::name() const
{
    if (!cdc_name())
        return acmacs::string::join(acmacs::string::join_slash, host(), location(), isolation(), year_fast());
    else
        return acmacs::string::join(acmacs::string::join_space, location(), isolation()); // cdc name

} // hidb::bin::Antigen::name

// ----------------------------------------------------------------------

hidb::bin::date_t hidb::bin::Antigen::make_date(std::string_view aDate)
{
    std::string compacted;
    if (aDate.size() == 10 && aDate[4] == '-' && aDate[7] == '-')
        compacted = std::string(aDate.substr(0, 4)) + std::string(aDate.substr(5, 2)) + std::string(aDate.substr(8, 2));
    else if (aDate.size() == 8)
        compacted = aDate;
    else
        throw invalid_date{};
    return static_cast<date_t>(stoul(compacted));

} // hidb::bin::Antigen::make_date

// ----------------------------------------------------------------------

hidb::bin::date_t hidb::bin::Antigen::date_raw() const
{
    if (date_offset == table_index_offset)
        return min_date();
    return *reinterpret_cast<const date_t*>(_start() + date_offset);

} // hidb::bin::Antigen::date_raw

// ----------------------------------------------------------------------

std::string hidb::bin::Antigen::date(bool compact) const
{
    if (date_offset == table_index_offset)
        return {};
    const std::string brief = std::to_string(*reinterpret_cast<const date_t*>(_start() + date_offset));
    if (brief.size() != 8)
        return brief;
    return compact ? brief : acmacs::string::join(acmacs::string::join_dash, std::string_view(brief.data(), 4), std::string_view(brief.data() + 4, 2), std::string_view(brief.data() + 6, 2));

} // hidb::bin::Antigen::date

// ----------------------------------------------------------------------

#ifndef __clang__
#pragma GCC push_options
// g++-7 and g++-8 fails on optimizing the loop in the function below even with -O1, loop becomes infinite
// possible reason is sizeof(lab_id_offset) = 5
#pragma GCC optimize("O0")
#endif

std::vector<std::string_view> hidb::bin::Antigen::lab_ids() const
{
    std::vector<std::string_view> result;
    for (size_t no = 0; no < sizeof(lab_id_offset); ++no) {
          // ignore padding after lab id
        const auto* start = _start() + lab_id_offset[no];
        auto* end = _start() + lab_id_offset[no+1];
        while (end > start && !end[-1])
            --end;
        if (end > start)
            result.emplace_back(start, static_cast<size_t>(end - start));
    }
    return result;

} // hidb::bin::Antigen::lab_ids

#ifndef __clang__
#pragma GCC pop_options
#endif

// ----------------------------------------------------------------------

#ifndef __clang__
#pragma GCC push_options
// g++ optimization bug (O2 leads to seg fault)
#pragma GCC optimize("O1")
#endif

std::vector<std::string_view> hidb::bin::Antigen::annotations() const
{
    std::vector<std::string_view> result;
    for (size_t no = 0; no < sizeof(annotation_offset); ++no) {
        if (const auto size = annotation_offset[no+1] - annotation_offset[no]; size > 0)
            result.emplace_back(_start() + annotation_offset[no], static_cast<size_t>(size));
    }
    return result;

} // hidb::bin::Antigen::annotations

#ifndef __clang__
#pragma GCC pop_options
#endif

// ----------------------------------------------------------------------

std::string hidb::bin::Serum::name() const
{
      // host, location, year are empty if name was not recognized
    return acmacs::string::join(acmacs::string::join_slash, host(), location(), isolation(), std::string_view(year()));

} // hidb::bin::Serum::name

// ----------------------------------------------------------------------

#ifndef __clang__
#pragma GCC push_options
// g++ optimization bug (O2 leads to seg fault)
#pragma GCC optimize("O1")
#endif

std::vector<std::string_view> hidb::bin::Serum::annotations() const
{
    std::vector<std::string_view> result;
    for (size_t no = 0; no < sizeof(annotation_offset); ++no) {
        if (const auto size = annotation_offset[no+1] - annotation_offset[no]; size > 0)
            result.emplace_back(_start() + annotation_offset[no], static_cast<size_t>(size));
    }
    return result;

} // hidb::bin::Serum::annotations

#ifndef __clang__
#pragma GCC pop_options
#endif

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
