#ifndef EQEMU_EQ_STR_REPOSITORY_H
#define EQEMU_EQ_STR_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_eq_str_repository.h"

class EqStrRepository: public BaseEqStrRepository {
public:
	// The client's eqstr_us.txt lines after its header and count: the id, a space, the text, by id.
	static std::vector<std::string> GetEqStrFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT(`id`, ' ', `value`) FROM {} ORDER BY `id` ASC",
				TableName()
			)
		);

		for (auto row : results) {
			lines.emplace_back(row[0]);
		}

		return lines;
	}
};

#endif //EQEMU_EQ_STR_REPOSITORY_H
