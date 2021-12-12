terraform {
  backend "gcs" {
    bucket                      = "nagyben-tfstate"
    prefix                      = "terraform/state"
    impersonate_service_account = "github-ci@nagyben.iam.gserviceaccount.com"
  }
}

locals {
  terraform_service_account = "github-ci@nagyben.iam.gserviceaccount.com"
}

provider "google" {
  project         = "nagyben"
  region          = "europe-west2"
  zone            = "europe-west2-a"
  access_token    = data.google_service_account_access_token.default.access_token
  request_timeout = "60s"
}

provider "google" {
  alias = "impersonation"
  scopes = [
    "https://www.googleapis.com/auth/cloud-platform",
    "https://www.googleapis.com/auth/userinfo.email",
  ]
}

data "google_service_account_access_token" "default" {
  provider               = google.impersonation
  target_service_account = local.terraform_service_account
  scopes                 = ["userinfo-email", "cloud-platform"]
  lifetime               = "1200s"
}

