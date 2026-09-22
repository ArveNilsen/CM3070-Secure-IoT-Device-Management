#pragma once
#include "cJSON.h"
#include <utility>
#include <string>
#include <stdexcept>
#include <optional>

namespace dev {
	
/**
 * @brief RAII owner for cJSON* object tree.
 */
class JsonValue {
public:
		explicit JsonValue(cJSON* raw) noexcept : ptr_(raw) {}

		static JsonValue object() {
				return JsonValue(cJSON_CreateObject());
		}

		~JsonValue() {
				if (ptr_) {
						cJSON_Delete(ptr_);
				}
		}

		// Move only
		JsonValue(const JsonValue&)						 = delete;
		JsonValue& operator=(const JsonValue&) = delete;

		JsonValue(JsonValue&& other) noexcept
				: ptr_(std::exchange(other.ptr_, nullptr)) {}

		JsonValue& operator=(JsonValue&& other) noexcept {
				if (this != &other) {
						if (ptr_) cJSON_Delete(ptr_);
						ptr_ = std::exchange(other.ptr_, nullptr);
				}

				return *this;
		}

		// Raw access, if needed
		cJSON* get() const noexcept { return ptr_; }
		bool valid() const noexcept { return ptr_ != nullptr; }
		explicit operator bool() const noexcept { return valid(); }

		// --- Building JSON object ---

		void add_string(const char* key, const std::string& value) {
				require_valid();
				cJSON_AddStringToObject(ptr_, key, value.c_str());
		}

		void add_number(const char* key, double value) {
				require_valid();
				cJSON_AddNumberToObject(ptr_, key, value);
		}

		void add_bool(const char* key, bool value) {
				require_valid();
				cJSON_AddBoolToObject(ptr_, key, value);
		}

		// Concat
		void add_object(const char* key, JsonValue&& child) {
				require_valid();
				cJSON_AddItemToObject(ptr_, key, child.release());
		}

		// --- Reading value ---

		std::optional<std::string> get_string(const char* key) const {
				if (!ptr_) 
						return std::nullopt;
				cJSON* item = cJSON_GetObjectItem(ptr_, key);
				if (!cJSON_IsString(item)) 
						return std::nullopt;
				return std::string(item->valuestring);
		}

		std::optional<double> get_number(const char* key) const {
				if (!ptr_)
						return std::nullopt;
				cJSON* item = cJSON_GetObjectItem(ptr_, key);
				if (!cJSON_IsNumber(item))
						return std::nullopt;
				return item->valuedouble;
		}

		// --- Serialisation ---

		std::string dump_unformatted() const {
				require_valid();
				char* raw = cJSON_PrintUnformatted(ptr_);
				std::string result(raw);
				cJSON_free(raw);
				return result;	
		}

		static JsonValue parse(const std::string& text) {
				return JsonValue(cJSON_ParseWithLength(
							text.data(), text.size()));
		}

private:
		void require_valid() const {
				assert(ptr_ != nullptr);
		}

		// Release without delete, internal only
		cJSON* release() noexcept {
				return std::exchange(ptr_, nullptr);
		}

		cJSON* ptr_ = nullptr;
};

} // namespace dev
