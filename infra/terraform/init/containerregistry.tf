locals {
  ci_artifact_repo_roles = [
    "roles/artifactregistry.reader",
  ]
}

resource "google_artifact_registry_repository" "docker_repo_init" {
  provider = google-beta
  repository_id = "nagyben-cr"
  description = "Docker container registry"
  format = "DOCKER"
  location = var.location
}

resource "google_artifact_registry_repository_iam_member" "ci" {
  provider = google-beta
  count    = length(local.ci_artifact_repo_roles)

  location = google_artifact_registry_repository.docker_repo_init.location
  repository = google_artifact_registry_repository.docker_repo_init.name
  role   = local.ci_artifact_repo_roles[count.index]
  member = "serviceAccount:${var.ci_sa}"
}