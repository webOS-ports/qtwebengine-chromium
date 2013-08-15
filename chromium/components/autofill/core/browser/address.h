// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "components/autofill/core/browser/form_group.h"

namespace autofill {

// A form group that stores address information.
class Address : public FormGroup {
 public:
  Address();
  Address(const Address& address);
  virtual ~Address();

  Address& operator=(const Address& address);

  // FormGroup:
  virtual base::string16 GetRawInfo(ServerFieldType type) const OVERRIDE;
  virtual void SetRawInfo(ServerFieldType type,
                          const base::string16& value) OVERRIDE;
  virtual base::string16 GetInfo(const AutofillType& type,
                           const std::string& app_locale) const OVERRIDE;
  virtual bool SetInfo(const AutofillType& type,
                       const base::string16& value,
                       const std::string& app_locale) OVERRIDE;
  virtual void GetMatchingTypes(
      const base::string16& text,
      const std::string& app_locale,
      ServerFieldTypeSet* matching_types) const OVERRIDE;

 private:
  // FormGroup:
  virtual void GetSupportedTypes(
      ServerFieldTypeSet* supported_types) const OVERRIDE;

  // The address, sans country info.
  base::string16 line1_;
  base::string16 line2_;
  base::string16 city_;
  base::string16 state_;
  base::string16 zip_code_;

  // The ISO 3166 2-letter country code, or an empty string if there is no
  // country data specified for this address.
  std::string country_code_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_H_
