terraform {
  backend "gcs" {
    bucket = "nagyben-tfstate"
    prefix = "terraform/state"
  }
}
