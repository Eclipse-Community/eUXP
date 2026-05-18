/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/AddonManagerWebAPI.h"

#include "js/TypeDecls.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Navigator.h"
#include "nsContentUtils.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIPrincipal.h"
#include "nsNetUtil.h"
#include "xpcpublic.h"

namespace mozilla {

using namespace mozilla::dom;

#ifndef MOZ_THUNDERBIRD
#  define MOZ_AMO_HOSTNAME "addons.mozilla.org"
#  define MOZ_AMO_STAGE_HOSTNAME "addons.allizom.org"
#  define MOZ_AMO_DEV_HOSTNAME "addons-dev.allizom.org"
#else
#  define MOZ_AMO_HOSTNAME "addons.thunderbird.net"
#  define MOZ_AMO_STAGE_HOSTNAME "addons-stage.thunderbird.net"
#  undef MOZ_AMO_DEV_HOSTNAME
#endif

static bool IsValidHost(const nsACString& host)
{
  if (host.EqualsLiteral(MOZ_AMO_HOSTNAME)) {
    return true;
  }

  if (xpc::IsInAutomation()) {
    if (host.LowerCaseEqualsLiteral(MOZ_AMO_STAGE_HOSTNAME) ||
#ifdef MOZ_AMO_DEV_HOSTNAME
        host.LowerCaseEqualsLiteral(MOZ_AMO_DEV_HOSTNAME) ||
#endif
        host.LowerCaseEqualsLiteral("example.com")) {
      return true;
    }
  }

  return false;
}

bool
AddonManagerWebAPI::IsValidSite(nsIURI* uri)
{
  if (!uri) {
    return false;
  }

  bool isHttps = false;
  if (NS_FAILED(uri->SchemeIs("https", &isHttps)) || !isHttps) {
    bool isHttp = false;
    if (!(xpc::IsInAutomation() &&
          NS_SUCCEEDED(uri->SchemeIs("http", &isHttp)) && isHttp)) {
      return false;
    }
  }

  nsAutoCString host;
  if (NS_FAILED(uri->GetHost(host))) {
    return false;
  }

  return IsValidHost(host);
}

bool
AddonManagerWebAPI::IsAPIEnabled(JSContext* aCx, JSObject* aGlobal)
{
  MOZ_DIAGNOSTIC_ASSERT(JS_IsGlobalObject(aGlobal));

  nsCOMPtr<nsPIDOMWindowInner> win = Navigator::GetWindowFromGlobal(aGlobal);
  if (!win) {
    return false;
  }

  while (win) {
    nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(win);
    if (!sop) {
      return false;
    }

    nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();
    if (!principal) {
      return false;
    }

    if (nsContentUtils::IsSystemPrincipal(principal)) {
      return true;
    }

    if (!IsValidSite(win->GetDocumentURI())) {
      return false;
    }

    nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();
    if (!docShell) {
      return false;
    }

    nsCOMPtr<nsIDocShellTreeItem> parent;
    if (NS_FAILED(docShell->GetSameTypeParent(getter_AddRefs(parent)))) {
      return false;
    }

    if (!parent) {
      return true;
    }

    return false;
  }

  return false;
}

namespace dom {

bool
AddonManagerPermissions::IsHostPermitted(const GlobalObject&, const nsAString& host)
{
  return IsValidHost(NS_ConvertUTF16toUTF8(host));
}

}  // namespace dom

}  // namespace mozilla