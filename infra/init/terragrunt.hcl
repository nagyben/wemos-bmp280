terraform {
  source = "${get_terragrunt_dir()}/../terraform//init"
}

include "root" {
  path   = find_in_parent_folders()
  expose = true
}

inputs = {
  ci_sa               = include.root.locals.terraform_service_account
  location            = "europe-west2"
}