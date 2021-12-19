locals {
  terraform_service_account = "github-ci@nagyben.iam.gserviceaccount.com"
}

remote_state {
  backend = "gcs"
  generate = {
    path      = "backend.tf"
    if_exists = "overwrite"
  }
  config = {
    bucket                      = "nagyben-tfstate"
    prefix                      = "terraform/${path_relative_to_include()}"
    impersonate_service_account = local.terraform_service_account
  }
}

inputs = {
  impersonate_service_account_email = local.terraform_service_account
}

generate "provider" {
  path = "provider.tf"
  if_exists = "overwrite_terragrunt"
  contents = <<EOF

variable "impersonate_service_account_email" {
  type = string
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
  target_service_account = var.impersonate_service_account_email
  scopes                 = ["userinfo-email", "cloud-platform"]
  lifetime               = "1200s"
}
EOF

}