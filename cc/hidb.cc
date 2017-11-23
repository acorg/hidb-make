#include <algorithm>

#include "acmacs-base/string.hh"
#include "acmacs-base/virus-name.hh"
#include "hidb-5/hidb.hh"
#include "hidb-5/hidb-bin.hh"
#include "hidb-5/hidb-json.hh"

// ----------------------------------------------------------------------

hidb::HiDb::HiDb(std::string aFilename, report_time timer)
{
    Timeit ti("reading hidb from " + aFilename + ": ", timer);
    acmacs::file::read_access access(aFilename);
    if (hidb::bin::has_signature(access.data())) {
        mAccess = std::move(access);
        mData = mAccess.data();
    }
    else if (std::string data = access; data.find("\"  version\": \"hidb-v5\"") != std::string::npos) {
        mDataStorage = hidb::json::read(data);
        mData = mDataStorage.data();
    }
    else
        throw std::runtime_error("[hidb] unrecognized file: " + aFilename);

} // hidb::HiDb::HiDb

// ----------------------------------------------------------------------

void hidb::HiDb::save(std::string aFilename) const
{
    if (!mDataStorage.empty())
        acmacs::file::write(aFilename, mDataStorage);
    else if (mAccess.valid())
        acmacs::file::write(aFilename, {mAccess.data(), mAccess.size()});

} // hidb::HiDb::save

// ----------------------------------------------------------------------

std::shared_ptr<hidb::Antigens> hidb::HiDb::antigens() const
{
    const auto* antigens = mData + reinterpret_cast<const hidb::bin::Header*>(mData)->antigen_offset;
    const auto number_of_antigens = *reinterpret_cast<const hidb::bin::ast_number_t*>(antigens);
    return std::make_shared<hidb::Antigens>(static_cast<size_t>(number_of_antigens),
                                            antigens + sizeof(hidb::bin::ast_number_t),
                                            antigens + sizeof(hidb::bin::ast_number_t) + sizeof(hidb::bin::ast_offset_t) * (number_of_antigens + 1));

} // hidb::HiDb::antigens

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::Antigen> hidb::Antigens::operator[](size_t aIndex) const
{
    return std::make_shared<hidb::Antigen>(mAntigen0 + reinterpret_cast<const hidb::bin::ast_offset_t*>(mIndex)[aIndex]);

} // hidb::Antigens::operator[]

// ----------------------------------------------------------------------

template <typename AgSr> inline std::pair<const hidb::bin::ast_offset_t*, const hidb::bin::ast_offset_t*> find_location(const hidb::bin::ast_offset_t* first, size_t number, const char* data, std::string location)
{
    const auto last = first + number;
    const auto found = std::lower_bound(
        first, last, location,
        [data](hidb::bin::ast_offset_t offset, const std::string& look_for) -> bool { return reinterpret_cast<const AgSr*>(data + offset)->location() < look_for; });
    for (auto end = found; end != last; ++end) {
        if (reinterpret_cast<const hidb::bin::Antigen*>(data + *end)->location() != location)
            return {found, end};
    }
    return {found, last};
}

// ----------------------------------------------------------------------

hidb::indexes_t hidb::Antigens::find(std::string aName) const
{
    std::string virus_type, host, location, isolation, year, passage;
    virus_name::split(aName, virus_type, host, location, isolation, year, passage);

    const auto* index_begin = reinterpret_cast<const hidb::bin::ast_offset_t*>(mIndex);
    const auto [first, last] = find_location<hidb::bin::Antigen>(index_begin, mNumberOfAntigens, mAntigen0, location);
    indexes_t result(static_cast<size_t>(last - first));
    std::transform(first, last, result.begin(), [index_begin](const hidb::bin::ast_offset_t& offset_ptr) -> size_t { return static_cast<size_t>(&offset_ptr - index_begin); });
    return result;

} // hidb::Antigens::find

// ----------------------------------------------------------------------

acmacs::chart::Name hidb::Antigen::name() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->name();

} // hidb::Antigen::name

// ----------------------------------------------------------------------

acmacs::chart::Date hidb::Antigen::date() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->date();

} // hidb::Antigen::date

// ----------------------------------------------------------------------

acmacs::chart::Passage hidb::Antigen::passage() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->passage();

} // hidb::Antigen::passage

// ----------------------------------------------------------------------

acmacs::chart::BLineage hidb::Antigen::lineage() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->lineage;

} // hidb::Antigen::lineage

// ----------------------------------------------------------------------

acmacs::chart::Reassortant hidb::Antigen::reassortant() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->reassortant();

} // hidb::Antigen::reassortant

// ----------------------------------------------------------------------

acmacs::chart::LabIds hidb::Antigen::lab_ids() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->lab_ids();

} // hidb::Antigen::lab_ids

// ----------------------------------------------------------------------

acmacs::chart::Annotations hidb::Antigen::annotations() const
{
    return reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->annotations();

} // hidb::Antigen::annotations

// ----------------------------------------------------------------------

hidb::indexes_t hidb::Antigen::tables() const
{
    const auto [size, ptr] = reinterpret_cast<const hidb::bin::Antigen*>(mAntigen)->tables();
    indexes_t result(size);
    std::transform(ptr, ptr + size, result.begin(), [](const auto& index) -> size_t { return static_cast<size_t>(index); });
    return result;

} // hidb::Antigen::tables

// ----------------------------------------------------------------------

std::shared_ptr<hidb::Sera> hidb::HiDb::sera() const
{
    const auto* sera = mData + reinterpret_cast<const hidb::bin::Header*>(mData)->serum_offset;
    const auto number_of_sera = *reinterpret_cast<const hidb::bin::ast_number_t*>(sera);
    return std::make_shared<hidb::Sera>(static_cast<size_t>(number_of_sera),
                                        sera + sizeof(hidb::bin::ast_number_t),
                                        sera + sizeof(hidb::bin::ast_number_t) + sizeof(hidb::bin::ast_offset_t) * (number_of_sera + 1));

} // hidb::HiDb::sera

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::Serum> hidb::Sera::operator[](size_t aIndex) const
{
    return std::make_shared<hidb::Serum>(mSerum0 + reinterpret_cast<const hidb::bin::ast_offset_t*>(mIndex)[aIndex]);

} // hidb::Sera::operator[]

// ----------------------------------------------------------------------

acmacs::chart::Name hidb::Serum::name() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->name();

} // hidb::Serum::name

// ----------------------------------------------------------------------

acmacs::chart::Passage hidb::Serum::passage() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->passage();

} // hidb::Serum::passsre

// ----------------------------------------------------------------------

acmacs::chart::BLineage hidb::Serum::lineage() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->lineage;

} // hidb::Serum::lineage

// ----------------------------------------------------------------------

acmacs::chart::Reassortant hidb::Serum::reassortant() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->reassortant();

} // hidb::Serum::reassortant

// ----------------------------------------------------------------------

acmacs::chart::Annotations hidb::Serum::annotations() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->annotations();

} // hidb::Serum::annotations

// ----------------------------------------------------------------------

acmacs::chart::SerumId hidb::Serum::serum_id() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->serum_id();

} // hidb::Serum::serum_id

// ----------------------------------------------------------------------

acmacs::chart::SerumSpecies hidb::Serum::serum_species() const
{
    return reinterpret_cast<const hidb::bin::Serum*>(mSerum)->serum_species();

} // hidb::Serum::serum_species

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList hidb::Serum::homologous_antigens() const
{
    const auto [size, ptr] = reinterpret_cast<const hidb::bin::Serum*>(mSerum)->homologous_antigens();
    acmacs::chart::PointIndexList result(size);
    std::transform(ptr, ptr + size, result.begin(), [](const auto& index) -> size_t { return static_cast<size_t>(index); });
    return result;

} // hidb::Serum::homologous_antigens

// ----------------------------------------------------------------------

hidb::indexes_t hidb::Serum::tables() const
{
    const auto [size, ptr] = reinterpret_cast<const hidb::bin::Serum*>(mSerum)->tables();
    indexes_t result(size);
    std::transform(ptr, ptr + size, result.begin(), [](const auto& index) -> size_t { return static_cast<size_t>(index); });
    return result;

} // hidb::Serum::tables

// ----------------------------------------------------------------------

std::shared_ptr<hidb::Tables> hidb::HiDb::tables() const
{
    const auto* tables = mData + reinterpret_cast<const hidb::bin::Header*>(mData)->table_offset;
    const auto number_of_tables = *reinterpret_cast<const hidb::bin::ast_number_t*>(tables);
    return std::make_shared<hidb::Tables>(static_cast<size_t>(number_of_tables),
                                          tables + sizeof(hidb::bin::ast_number_t),
                                          tables + sizeof(hidb::bin::ast_number_t) + sizeof(hidb::bin::ast_offset_t) * (number_of_tables + 1));

} // hidb::HiDb::tables

// ----------------------------------------------------------------------

std::shared_ptr<hidb::Table> hidb::Tables::operator[](size_t aIndex) const
{
    return std::make_shared<hidb::Table>(mTable0 + reinterpret_cast<const hidb::bin::ast_offset_t*>(mIndex)[aIndex]);

} // hidb::Tables::operator[]

// ----------------------------------------------------------------------

std::string hidb::Table::name() const
{
    return string::join(":", {lab(), assay(), acmacs::chart::BLineage(mTable->lineage), rbc(), date()});

} // hidb::Table::name

// ----------------------------------------------------------------------

std::string hidb::Table::assay() const
{
    return mTable->assay();

} // hidb::Table::assay

// ----------------------------------------------------------------------

std::string hidb::Table::lab() const
{
    return mTable->lab();

} // hidb::Table::lab

// ----------------------------------------------------------------------

std::string hidb::Table::date() const
{
    return mTable->date();

} // hidb::Table::date

// ----------------------------------------------------------------------

std::string hidb::Table::rbc() const
{
    return mTable->rbc();

} // hidb::Table::rbc

// ----------------------------------------------------------------------

size_t hidb::Table::number_of_antigens() const
{
    return mTable->number_of_antigens();

} // hidb::Table::number_of_antigens

// ----------------------------------------------------------------------

size_t hidb::Table::number_of_sera() const
{
    return mTable->number_of_sera();

} // hidb::Table::number_of_sera

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
