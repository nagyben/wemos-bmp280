terraform {
  source = "${get_terragrunt_dir()}/../terraform//app"

  before_hook "export_viz_requirements" {
    commands = ["apply"]
    execute = ["poetry", "export", "--without-hashes", "-o", "requirements.txt"]
    working_dir = "${get_terragrunt_dir()}/../../cloud_functions/viz"
  }

  before_hook "export_receiver_requirements" {
    commands = ["apply"]
    execute = ["poetry", "export", "--without-hashes", "-o", "requirements.txt"]
    working_dir = "${get_terragrunt_dir()}/../../cloud_functions/receiver"
  }
}

include "root" {
  path = find_in_parent_folders()
  expose = true
}

inputs = {
  env_suffix = ""
  ci_sa = include.root.locals.terraform_service_account
}

