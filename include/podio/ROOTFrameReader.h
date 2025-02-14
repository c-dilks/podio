#ifndef PODIO_ROOTFRAMEREADER_H
#define PODIO_ROOTFRAMEREADER_H

#include "podio/CollectionBranches.h"
#include "podio/ROOTFrameData.h"
#include "podio/podioVersion.h"
#include "podio/utilities/DatamodelRegistryIOHelpers.h"

#include "TChain.h"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

// forward declarations
class TClass;
// class TChain;
class TFile;
class TTree;

namespace podio {

namespace detail {
  // Information about the collection class type, whether it is a subset, the
  // schema version on file and the index in the collection branches cache
  // vector
  using CollectionInfo = std::tuple<std::string, bool, SchemaVersionT, size_t>;

} // namespace detail

class EventStore;
class CollectionBase;
class CollectionIDTable;
class GenericParameters;
struct CollectionReadBuffers;

/**
 * This class has the function to read available data from disk
 * and to prepare collections and buffers.
 **/
class ROOTFrameReader {

public:
  ROOTFrameReader() = default;
  ~ROOTFrameReader() = default;

  // non-copyable
  ROOTFrameReader(const ROOTFrameReader&) = delete;
  ROOTFrameReader& operator=(const ROOTFrameReader&) = delete;

  /**
   * Open a single file for reading.
   *
   * @param filename The name of the input file
   */
  void openFile(const std::string& filename);

  /**
   * Open multiple files for reading and then treat them as if they are one file
   *
   * NOTE: All of the files are assumed to have the same structure. Specifically
   * this means:
   * - The same categories are available from all files
   * - The collections that are contained in the individual categories are the
   *   same across all files
   *
   * This usually boils down to "the files have been written with the same
   * settings", e.g. they are outputs of a batched process.
   *
   * @param filenames The filenames of all input files that should be read
   */
  void openFiles(const std::vector<std::string>& filenames);

  /**
   * Read the next data entry from which a Frame can be constructed for the
   * given name. In case there are no more entries left for this name or in
   * case there is no data for this name, this returns a nullptr.
   */
  std::unique_ptr<podio::ROOTFrameData> readNextEntry(const std::string& name);

  /**
   * Read the specified data entry from which a Frame can be constructed for
   * the given name. In case the entry does not exist for this name or in case
   * there is no data for this name, this returns a nullptr.
   */
  std::unique_ptr<podio::ROOTFrameData> readEntry(const std::string& name, const unsigned entry);

  /// Returns number of entries for the given name
  unsigned getEntries(const std::string& name) const;

  /// Get the build version of podio that has been used to write the current file
  podio::version::Version currentFileVersion() const {
    return m_fileVersion;
  }

  /// Get the names of all the availalable Frame categories in the current file(s)
  std::vector<std::string_view> getAvailableCategories() const;

  /// Get the datamodel definition for the given name
  const std::string_view getDatamodelDefinition(const std::string& name) const {
    return m_datamodelHolder.getDatamodelDefinition(name);
  }

  /// Get all names of the datamodels that ara available from this reader
  std::vector<std::string> getAvailableDatamodels() const {
    return m_datamodelHolder.getAvailableDatamodels();
  }

private:
  /**
   * Helper struct to group together all the necessary state to read / process a
   * given category. A "category" in this case describes all frames with the
   * same name which are constrained by the ROOT file structure that we use to
   * have the same contents. It encapsulates all state that is necessary for
   * reading from a TTree / TChain (i.e. collection infos, branches, ...)
   */
  struct CategoryInfo {
    /// constructor from chain for more convenient map insertion
    CategoryInfo(std::unique_ptr<TChain>&& c) : chain(std::move(c)) {
    }
    std::unique_ptr<TChain> chain{nullptr};                                      ///< The TChain with the data
    unsigned entry{0};                                                           ///< The next entry to read
    std::vector<std::pair<std::string, detail::CollectionInfo>> storedClasses{}; ///< The stored collections in this
                                                                                 ///< category
    std::vector<root_utils::CollectionBranches> branches{};                      ///< The branches for this category
    std::shared_ptr<CollectionIDTable> table{nullptr}; ///< The collection ID table for this category
  };

  /**
   * Initialze the passed CategoryInfo by setting up the necessary branches,
   * collection infos and all necessary meta data to be able to read entries
   * with this name
   */
  void initCategory(CategoryInfo& catInfo, const std::string& name);

  /**
   * Get the category information for the given name. In case there is no TTree
   * with contents for the given name this will return a CategoryInfo with an
   * uninitialized chain (nullptr) member
   */
  CategoryInfo& getCategoryInfo(const std::string& name);

  /**
   * Read the parameters for the entry specified in the passed CategoryInfo
   */
  GenericParameters readEntryParameters(CategoryInfo& catInfo, bool reloadBranches, unsigned int localEntry);

  /**
   * Read the data entry specified in the passed CategoryInfo, and increase the
   * counter aferwards. In case the requested entry is larger than the
   * available number of entries, return a nullptr.
   */
  std::unique_ptr<podio::ROOTFrameData> readEntry(ROOTFrameReader::CategoryInfo& catInfo);

  /**
   * Get / read the buffers at index iColl in the passed category information
   */
  podio::CollectionReadBuffers getCollectionBuffers(CategoryInfo& catInfo, size_t iColl, bool reloadBranches,
                                                    unsigned int localEntry);

  std::unique_ptr<TChain> m_metaChain{nullptr};                 ///< The metadata tree
  std::unordered_map<std::string, CategoryInfo> m_categories{}; ///< All categories
  std::vector<std::string> m_availCategories{};                 ///< All available categories from this file

  podio::version::Version m_fileVersion{0, 0, 0};
  DatamodelDefinitionHolder m_datamodelHolder{};
};

} // namespace podio

#endif // PODIO_ROOTFRAMEREADER_H
